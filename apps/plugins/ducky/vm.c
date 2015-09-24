#include "ducky.h"
#include "fixedpoint.h"
#include "tlsf.h"

#define STACK_SZ 1024
#define CALLSTACK_SZ 64
#define MAX_VARS 1024

static imm_t callstack[CALLSTACK_SZ];
static unsigned callstack_pointer;

static imm_t stack[STACK_SZ];
static unsigned stack_pointer;

struct var_t {
    vartype val;
    bool constant;
};

static struct var_t vars[MAX_VARS];
static unsigned current_line, num_lines;

static off_t *line_offset;

static bool want_quit;
static int repeats_left;
static imm_t repeat_line;

static int vm_keystate;
static unsigned vm_nkeys;

instr_t read_instr(void)
{
    instr_t ret;
    if(rb->read(file_des, &ret, sizeof(ret)) != sizeof(ret))
        want_quit = true;
    return ret;
}

imm_t read_imm(void)
{
    imm_t ret;
    if(rb->read(file_des, &ret, sizeof(ret)) != sizeof(ret))
        want_quit = true;
    return ret;
}

imm_t read_varid(void)
{
    varid_t ret;
    if(rb->read(file_des, &ret, sizeof(ret)) != sizeof(ret))
        want_quit = true;
    return ret;
}

imm_t read_byte(void)
{
    unsigned char ret;
    if(rb->read(file_des, &ret, sizeof(ret)) != sizeof(ret))
        want_quit = true;
    return ret;
}

static void init_globals(void)
{
    current_line = 0;
    stack_pointer = 0;
    callstack_pointer = 0;
    want_quit = false;
    repeats_left = 0;
    repeat_line = 0;
    keys_sent = 0;
    vm_keystate = 0;
    vm_nkeys = 0;
}

static inline void push(imm_t n)
{
    if(stack_pointer < STACK_SZ)
        stack[stack_pointer++] = n;
    else
        error("stack overflow");
}

static inline imm_t pop(void)
{
    if(stack_pointer > 0)
        return stack[--stack_pointer];
    else
        error("stack underflow");
}

static inline vartype getvar(varid_t varid)
{
    if(varid < ARRAYLEN(vars))
        return vars[varid].val;
    else
        error("variable out of range");
}

static inline void setvar(varid_t varid, vartype val)
{
    if(varid < ARRAYLEN(vars) && !vars[varid].constant)
        vars[varid].val = val;
    else
        error("cannot modify variable");
}

static inline void mkconst(varid_t varid)
{
    if(varid < ARRAYLEN(vars))
        vars[varid].constant = true;
}

static inline void jump(imm_t line)
{
    if(1 <= line && line <= num_lines)
    {
        rb->lseek(file_des, line_offset[line], SEEK_SET);
        current_line = line;
    }
    else
        error("jump target out of bounds");
}

static void pushimm_handler(void)
{
    push(read_imm());
}

static void pushvar_handler(void)
{
    push(getvar(read_varid()));
}

static void pop_handler(void)
{
    setvar(read_varid(), pop());
}

static void mkconst_handler(void)
{
    mkconst(read_varid());
}

static void incvar_handler(void)
{
    varid_t varid = read_varid();
    if(varid < ARRAYLEN(vars))
        ++vars[varid].val;
}

static void decvar_handler(void)
{
    varid_t varid = read_varid();
    if(varid < ARRAYLEN(vars))
        --vars[varid].val;
}

static void writestr_handler(void)
{
    while(1)
    {
        char c = read_byte();
        if(c)
            vid_writef_no_nl("%c", c);
        else
            break;
    }
}

static void repeat_handler(void)
{
    if(repeats_left > 0)
    {
        if(repeat_line + 1 != current_line)
            error("nested REPEAT");
        --repeats_left;
        if(repeats_left > 0)
        {
            jump(repeat_line);
        }
    }
    else
    {
        repeat_line = pop();
        repeats_left = pop() - 1;
        jump(repeat_line);
    }
}

static void jump_handler(void)
{
    jump(pop());
}

static void subcall_handler(void)
{
    if(callstack_pointer < CALLSTACK_SZ)
    {
        callstack[callstack_pointer++] = current_line + 1;
        jump(pop());
    }
    else
        error("call stack overflow");
}

static void subret_handler(void)
{
    if(callstack_pointer > 0)
    {
        jump(callstack[--callstack_pointer]);
    }
    else
        error("call stack underflow");
}

static void if_handler(void)
{
    imm_t line = pop();
    if(!pop())
        jump(line);
}

static void delay_handler(void)
{
    imm_t ms = pop();

    rb->sleep(ms/10);
}

static void logvar_handler(void)
{
    vid_writef(VARFORMAT, pop());
}

static void quit_handler(void)
{
    want_quit = true;
}

static void logascii_handler(void)
{
    vid_writef("%c", (char)pop());
}

static void neg_handler(void)
{
    stack[stack_pointer - 1] = -stack[stack_pointer - 1];
}

static vartype eval_exp(vartype a1, vartype a2)
{
    return a2<0 ? 0 : (a2==0?1:a1*eval_exp(a1, a2-1));
}

static void pow_handler(void)
{
    imm_t pow = pop();
    imm_t base = pop();
    push(eval_exp(base, pow));
}

static void mul_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a*b);
}

static void div_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a/b);
}

static void mod_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a%b);
}

static void add_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a+b);
}

static void sub_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a-b);
}

static void eq_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a==b);
}

static void neq_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a!=b);
}

static void leq_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a<=b);
}

static void geq_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a>=b);
}

static void lt_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a<b);
}

static void gt_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a>b);
}

static void lognot_handler(void)
{
    push(!pop());
}

static void logand_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a&&b);
}

static void logor_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a||b);
}

static void bitand_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a&b);
}

static void bitor_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a|b);
}

static void bitxor_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a^b);
}

static void bitcomp_handler(void)
{
    push(~pop());
}

static void lsh_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a<<b);
}

static void rsh_handler(void)
{
    imm_t b = pop();
    imm_t a = pop();
    push(a>>b);
}

static void sqrt_handler(void)
{
    push(isqrt(pop()) + 1);
}

static void decl_const(void)
{
    /* no checking, only the compiler can output this instruction */
    varid_t varid = read_varid();
    vars[varid].val = read_imm();
    vars[varid].constant = true;
}

static void sendkey_handler(void)
{
    add_key(&vm_keystate, &vm_nkeys, pop());
}

static void dfl_delay_handler(void)
{
    default_delay = pop() / 10;
}

static void str_delay_handler(void)
{
    string_delay = pop() / 10;
}

static void type_dec_handler(void)
{
    char str[32];
    rb->snprintf(str, sizeof(str), VARFORMAT, pop());
    send_string(str);
}

static void send_string_file(void)
{
    while(1)
    {
        int string_state = 0;
        char c = read_byte();
        if(!c)
            break;
        add_char(&string_state, NULL, c);

        send(string_state);

        /* rb->sleep(0) sleeps till the next tick, which is not what
           we want */
        if(string_delay)
        {
            rb->sleep(string_delay);
            rb->yield();
        }
    }
}

static void type_str_handler(void)
{
    send_string_file();
}

static void addchar_handler(void)
{
    add_char(&vm_keystate, &vm_nkeys, read_byte());
}

static void newline_handler(void)
{
    /* LOG prints newline, so do nothing here */
    return;
}

static void input_handler(void)
{
    char buf[32];
    rb->memset(buf, 0, sizeof(buf));
    if(rb->kbd_input(buf, sizeof(buf)) < 0)
        setvar(read_varid(), 0);
    else
    {
        setvar(read_varid(), strtol(buf, NULL, 0));
        /* print the user's input */
        vid_writef("%s", buf);
    }
}

static void inc_line_pointer(void)
{
    if(vm_keystate)
        send(vm_keystate);
    vm_keystate = 0;
    vm_nkeys = 0;
    ++current_line;

    vars[0].val = current_line;
}

static void (*instr_tab[0x100])(void) = {
    pushimm_handler,   /*  0x0   */
    pushvar_handler,   /*  0x1   */
    pop_handler,       /*  0x2   */
    mkconst_handler,   /*  0x3   */
    incvar_handler,    /*  0x4   */
    decvar_handler,    /*  0x5   */
    writestr_handler,  /*  0x6   */
    repeat_handler,    /*  0x7   */
    jump_handler,      /*  0x8   */
    subcall_handler,   /*  0x9   */
    subret_handler,    /*  0xa   */
    if_handler,        /*  0xb   */
    delay_handler,     /*  0xc   */
    logvar_handler,    /*  0xd   */
    quit_handler,      /*  0xe   */
    logascii_handler,  /*  0xf   */
    neg_handler,       /*  0x10  */
    pow_handler,       /*  0x11  */
    mul_handler,       /*  0x12  */
    div_handler,       /*  0x13  */
    mod_handler,       /*  0x14  */
    add_handler,       /*  0x15  */
    sub_handler,       /*  0x16  */
    eq_handler,        /*  0x17  */
    neq_handler,       /*  0x18  */
    leq_handler,       /*  0x19  */
    geq_handler,       /*  0x1a  */
    lt_handler,        /*  0x1b  */
    gt_handler,        /*  0x1c  */
    lognot_handler,    /*  0x1d  */
    logand_handler,    /*  0x1e  */
    logor_handler,     /*  0x1f  */
    bitand_handler,    /*  0x20  */
    bitor_handler,     /*  0x21  */
    bitxor_handler,    /*  0x22  */
    bitcomp_handler,   /*  0x23  */
    lsh_handler,       /*  0x24  */
    rsh_handler,       /*  0x25  */
    sqrt_handler,      /*  0x26  */
    decl_const,        /*  0x27  */
    sendkey_handler,   /*  0x28  */
    dfl_delay_handler, /*  0x29  */
    str_delay_handler, /*  0x2a  */
    type_dec_handler,  /*  0x2b  */
    type_str_handler,  /*  0x2c  */
    addchar_handler,   /*  0x2d  */
    newline_handler,   /*  0x2e  */
    input_handler,     /*  0x2f  */
    NULL,              /*  0x30  */
    NULL,              /*  0x31  */
    NULL,              /*  0x32  */
    NULL,              /*  0x33  */
    NULL,              /*  0x34  */
    NULL,              /*  0x35  */
    NULL,              /*  0x36  */
    NULL,              /*  0x37  */
    NULL,              /*  0x38  */
    NULL,              /*  0x39  */
    NULL,              /*  0x3a  */
    NULL,              /*  0x3b  */
    NULL,              /*  0x3c  */
    NULL,              /*  0x3d  */
    NULL,              /*  0x3e  */
    NULL,              /*  0x3f  */
    NULL,              /*  0x40  */
    NULL,              /*  0x41  */
    NULL,              /*  0x42  */
    NULL,              /*  0x43  */
    NULL,              /*  0x44  */
    NULL,              /*  0x45  */
    NULL,              /*  0x46  */
    NULL,              /*  0x47  */
    NULL,              /*  0x48  */
    NULL,              /*  0x49  */
    NULL,              /*  0x4a  */
    NULL,              /*  0x4b  */
    NULL,              /*  0x4c  */
    NULL,              /*  0x4d  */
    NULL,              /*  0x4e  */
    NULL,              /*  0x4f  */
    NULL,              /*  0x50  */
    NULL,              /*  0x51  */
    NULL,              /*  0x52  */
    NULL,              /*  0x53  */
    NULL,              /*  0x54  */
    NULL,              /*  0x55  */
    NULL,              /*  0x56  */
    NULL,              /*  0x57  */
    NULL,              /*  0x58  */
    NULL,              /*  0x59  */
    NULL,              /*  0x5a  */
    NULL,              /*  0x5b  */
    NULL,              /*  0x5c  */
    NULL,              /*  0x5d  */
    NULL,              /*  0x5e  */
    NULL,              /*  0x5f  */
    NULL,              /*  0x60  */
    NULL,              /*  0x61  */
    NULL,              /*  0x62  */
    NULL,              /*  0x63  */
    NULL,              /*  0x64  */
    NULL,              /*  0x65  */
    NULL,              /*  0x66  */
    NULL,              /*  0x67  */
    NULL,              /*  0x68  */
    NULL,              /*  0x69  */
    NULL,              /*  0x6a  */
    NULL,              /*  0x6b  */
    NULL,              /*  0x6c  */
    NULL,              /*  0x6d  */
    NULL,              /*  0x6e  */
    NULL,              /*  0x6f  */
    NULL,              /*  0x70  */
    NULL,              /*  0x71  */
    NULL,              /*  0x72  */
    NULL,              /*  0x73  */
    NULL,              /*  0x74  */
    NULL,              /*  0x75  */
    NULL,              /*  0x76  */
    NULL,              /*  0x77  */
    NULL,              /*  0x78  */
    NULL,              /*  0x79  */
    NULL,              /*  0x7a  */
    NULL,              /*  0x7b  */
    NULL,              /*  0x7c  */
    NULL,              /*  0x7d  */
    NULL,              /*  0x7e  */
    NULL,              /*  0x7f  */
    NULL,              /*  0x80  */
    NULL,              /*  0x81  */
    NULL,              /*  0x82  */
    NULL,              /*  0x83  */
    NULL,              /*  0x84  */
    NULL,              /*  0x85  */
    NULL,              /*  0x86  */
    NULL,              /*  0x87  */
    NULL,              /*  0x88  */
    NULL,              /*  0x89  */
    NULL,              /*  0x8a  */
    NULL,              /*  0x8b  */
    NULL,              /*  0x8c  */
    NULL,              /*  0x8d  */
    NULL,              /*  0x8e  */
    NULL,              /*  0x8f  */
    NULL,              /*  0x90  */
    NULL,              /*  0x91  */
    NULL,              /*  0x92  */
    NULL,              /*  0x93  */
    NULL,              /*  0x94  */
    NULL,              /*  0x95  */
    NULL,              /*  0x96  */
    NULL,              /*  0x97  */
    NULL,              /*  0x98  */
    NULL,              /*  0x99  */
    NULL,              /*  0x9a  */
    NULL,              /*  0x9b  */
    NULL,              /*  0x9c  */
    NULL,              /*  0x9d  */
    NULL,              /*  0x9e  */
    NULL,              /*  0x9f  */
    NULL,              /*  0xa0  */
    NULL,              /*  0xa1  */
    NULL,              /*  0xa2  */
    NULL,              /*  0xa3  */
    NULL,              /*  0xa4  */
    NULL,              /*  0xa5  */
    NULL,              /*  0xa6  */
    NULL,              /*  0xa7  */
    NULL,              /*  0xa8  */
    NULL,              /*  0xa9  */
    NULL,              /*  0xaa  */
    NULL,              /*  0xab  */
    NULL,              /*  0xac  */
    NULL,              /*  0xad  */
    NULL,              /*  0xae  */
    NULL,              /*  0xaf  */
    NULL,              /*  0xb0  */
    NULL,              /*  0xb1  */
    NULL,              /*  0xb2  */
    NULL,              /*  0xb3  */
    NULL,              /*  0xb4  */
    NULL,              /*  0xb5  */
    NULL,              /*  0xb6  */
    NULL,              /*  0xb7  */
    NULL,              /*  0xb8  */
    NULL,              /*  0xb9  */
    NULL,              /*  0xba  */
    NULL,              /*  0xbb  */
    NULL,              /*  0xbc  */
    NULL,              /*  0xbd  */
    NULL,              /*  0xbe  */
    NULL,              /*  0xbf  */
    NULL,              /*  0xc0  */
    NULL,              /*  0xc1  */
    NULL,              /*  0xc2  */
    NULL,              /*  0xc3  */
    NULL,              /*  0xc4  */
    NULL,              /*  0xc5  */
    NULL,              /*  0xc6  */
    NULL,              /*  0xc7  */
    NULL,              /*  0xc8  */
    NULL,              /*  0xc9  */
    NULL,              /*  0xca  */
    NULL,              /*  0xcb  */
    NULL,              /*  0xcc  */
    NULL,              /*  0xcd  */
    NULL,              /*  0xce  */
    NULL,              /*  0xcf  */
    NULL,              /*  0xd0  */
    NULL,              /*  0xd1  */
    NULL,              /*  0xd2  */
    NULL,              /*  0xd3  */
    NULL,              /*  0xd4  */
    NULL,              /*  0xd5  */
    NULL,              /*  0xd6  */
    NULL,              /*  0xd7  */
    NULL,              /*  0xd8  */
    NULL,              /*  0xd9  */
    NULL,              /*  0xda  */
    NULL,              /*  0xdb  */
    NULL,              /*  0xdc  */
    NULL,              /*  0xdd  */
    NULL,              /*  0xde  */
    NULL,              /*  0xdf  */
    NULL,              /*  0xe0  */
    NULL,              /*  0xe1  */
    NULL,              /*  0xe2  */
    NULL,              /*  0xe3  */
    NULL,              /*  0xe4  */
    NULL,              /*  0xe5  */
    NULL,              /*  0xe6  */
    NULL,              /*  0xe7  */
    NULL,              /*  0xe8  */
    NULL,              /*  0xe9  */
    NULL,              /*  0xea  */
    NULL,              /*  0xeb  */
    NULL,              /*  0xec  */
    NULL,              /*  0xed  */
    NULL,              /*  0xee  */
    NULL,              /*  0xef  */
    NULL,              /*  0xf0  */
    NULL,              /*  0xf1  */
    NULL,              /*  0xf2  */
    NULL,              /*  0xf3  */
    NULL,              /*  0xf4  */
    NULL,              /*  0xf5  */
    NULL,              /*  0xf6  */
    NULL,              /*  0xf7  */
    NULL,              /*  0xf8  */
    NULL,              /*  0xf9  */
    NULL,              /*  0xfa  */
    NULL,              /*  0xfb  */
    NULL,              /*  0xfc  */
    NULL,              /*  0xfd  */
    NULL,              /*  0xfe  */
    inc_line_pointer,  /*  0xff  */
};

void ducky_vm(int fd)
{
    file_des = fd;

    init_globals();

    if(read_imm() != DUCKY_MAGIC)
        error("unknown format");

    num_lines = read_imm();
    line_offset = tlsf_malloc(num_lines + 1);
    for(unsigned int i = 1; i <= num_lines; ++i)
    {
        line_offset[i] = read_imm();
    }

    /* and... execute! */
    while(!want_quit)
    {
        instr_t instr = read_instr();
        if(want_quit)
            break;
        instr_tab[instr]();
    }
}

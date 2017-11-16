/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 William Wilgus
 *               2016 Moshe Piekarski - char output code prior work 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "lib/pluginlib_actions.h"
#include <stdio.h>

static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

/* Checks execution time between first call for output and first call for input */
//#define BF_EXEC_TIME_PROFILE

/* PROGRAM BUFFERS */
#define STACK_SIZE      128
#define DATA_SIZE       1024

#define PROG_BUFFER     32768

#define TEXT_BUFFER      180
#define INPUT_BUFFER     5

/* Brain Fuck Commands
>    increment the data pointer (to point to the next cell to the right).
<    decrement the data pointer (to point to the next cell to the left).
+    increment (increase by one) the byte at the data pointer.
-    decrement (decrease by one) the byte at the data pointer.
.    output the byte at the data pointer.
,    accept one byte of input, storing its value in the byte at the data pointer.
[    if the byte at the data pointer is zero, then instead of moving the
    ins pointer forward to the next command, jump it forward to the
    command after the matching ] command.
]    if the byte at the data pointer is nonzero, then instead of moving the
    ins pointer forward to the next command,    jump it back to the command after the matching [ command.
*/

#define BF_INC_DP       '>'
#define BF_DEC_DP       '<'
#define BF_INC_VAL      '+'
#define BF_DEC_VAL      '-'
#define BF_OUTPUT       '.'
#define BF_INPUT        ','
#define BF_JFZ          '['
#define BF_JBNZ         ']'

/* OPTIMIZED BF OPERATIONS*/
/* optimized programs turn long lists of operations into a single
    operation with an amount to increment/decrement by */
#define OP_INC_DP       '\x0E' //'I'//
#define OP_DEC_DP       '\x0F' //'D'//
#define OP_INC_VAL      '\x11' //'A'//
#define OP_DEC_VAL      '\x12' //'S'//
#define OP_ZERO_VAL     '\x18' //'Z'//
#define OP_ONE_VAL      '\x13' //'O'//
/* loop offsets are stored directly to file for faster jumps */
#define OP_LOOP_ZERO    '{'
#define OP_LOOP_NZERO    '}'

/*ERRORS*/
#define SUCCESS        0x000
#define USER_EXIT      0x001
#define ERROR          0x002
#define STACK_OVFL     0x004
#define STACK_UNFL     0x008
#define DATA_OVFL      0x010
#define DATA_UNFL      0x020
#define ENDOFFILE      0x040
#define INS_OVFL       0x080
#define FILE_ERR       0x100

/*STACK MANIPULATION stores return address of loops*/
#define STACK_EMPTY()   (stkptr <= 0)
#define STACK_FULL()    (stkptr >= STACK_SIZE)
#define STACK_PUSH(a)   (stack[stkptr++] = a)
#define STACK_POP()     (stack[--stkptr])
#define STACK_PEEK()     (stack[stkptr-1])
#define STACK_DISCARD()     (stkptr--)

const char prompt_continue[] = "Press Select to continue.";
const char op_header[] = "#BFOptimized\nDO NOT ALTER DEPENDS ON FILE OFFSETS\n\0";
static int8_t data[DATA_SIZE] = {0};
static uint32_t stack[STACK_SIZE] = {0};
static unsigned char program[PROG_BUFFER] = {0};
#ifdef BF_EXEC_TIME_PROFILE
uint32_t e_time = 0; //for profiling
#endif
static int button_prompt(bool prompt, int prompt_button, const char* prompt_text)
{
    int fh;
    int button = BUTTON_NONE;
    button = pluginlib_getaction(TIMEOUT_NOBLOCK,
                                 plugin_contexts,
                                 ARRAYLEN(plugin_contexts));
    if (prompt)
    {
        rb->lcd_getstringsize("W", NULL, &fh);
        rb->lcd_putsxy(3,LCD_HEIGHT-fh, prompt_text);
        rb->lcd_update();
        rb->sleep(HZ/2);

        while (1)
        {
            button = pluginlib_getaction(HZ,
                                         plugin_contexts,
                                         ARRAYLEN(plugin_contexts));
            if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                return PLA_EXIT;

            if (button == PLA_EXIT || button == prompt_button ||
                button != (prompt_button == -1 ? 0 : button))
            break;
        }
    }
    return button;
}

int output(bool new, const char *fmt, ...)
{
    /* output modified from midi */
    static int  p_xtpt = 1;
    static int  p_ytpt = 1;
    static int  h = 0;

    static bool prompt_next = false;
    static char p_buf[TEXT_BUFFER];
    static int  pos = 0;

    int         i = 0, p = 0;
    char        last_chr = '\0';
    int         chr_count = 0;
    int         new_xtpt = 0;
    int         fh;
    static int  fw = 1;
    va_list ap;

    if (new)
    {
    /* sets cursor to beginning of screen, if fmt[0] == '\0' clears buffer */
        p_ytpt = 1;
        p_xtpt = 1;
        new = false;
        if (fmt[0] == '\0')
        {
            pos = 0;
            prompt_next = false;
            return 3;
        }
    }
    va_start(ap, fmt);
    chr_count = rb->vsnprintf(&p_buf[pos],sizeof(p_buf)-pos, fmt, ap);
    va_end(ap);

    if (chr_count == 1 && (fmt[1] == 'c' || fmt[1] == 'C'))
    {
        new_xtpt = p_xtpt + rb->font_getstringsize(p_buf, NULL, NULL, FONT_UI);
        last_chr = p_buf[pos];
        if ((last_chr == ' ' || last_chr <= '\e') && (new_xtpt) > (LCD_WIDTH - fw * 3))
        {   /*print now*/
            p_buf[pos] = '\n';
            last_chr = '\n';
        }
        else if ((new_xtpt) >= (LCD_WIDTH) && pos+1 < TEXT_BUFFER)
            p_buf[++pos] = '\n';
        else if ((pos++ < TEXT_BUFFER) && last_chr > '\e')
            return 2; /*Continue building buffer*/
    }

    pos = 0;
    if (prompt_next)
    {
        if (button_prompt(true, PLA_SELECT, prompt_continue) == PLA_EXIT)
            return 0;
        p_xtpt=1;
        p_ytpt=1;
        //h = 0;
        rb->lcd_clear_display();
        prompt_next = false;
    }

    rb->lcd_getstringsize("W", &fw, &fh);

    /* Device LCDs display newlines funny. */
    for(i = 0; p_buf[i]!='\0'; i++)
        if (p_buf[i] == '\n')
        {
            p_buf[i] = '\0';
            rb->lcd_putsxy(p_xtpt,p_ytpt, (unsigned char *)&p_buf[p]);
            if (h ==0)
                rb->font_getstringsize(&p_buf[p], NULL, &h, FONT_UI);
            rb->lcd_update();
            p_ytpt+=h;//+2;
            p_xtpt = 1;
            p = i+1;
        }
    if (p_buf[p] != '\0')
    {
        rb->lcd_putsxy(p_xtpt,p_ytpt, (unsigned char *)&p_buf[p]);
        p_xtpt+= rb->font_getstringsize(&p_buf[p], NULL, &h, FONT_UI);
        rb->lcd_update();
        if (p_xtpt>LCD_WIDTH-fw)
        {
            p_ytpt+=h;//+2;
            p_xtpt=1;
        }
    }
    if (p_ytpt >= LCD_HEIGHT-(fh * 2))
        prompt_next = true;
    else
        h = 0;
    return 1;
}

int input(void)
{
    static char in[INPUT_BUFFER] = {'\n'}; // LF
    static unsigned char pos = 0;
    int ret = 1;
    in[0] = '\0';
    int out = '\n'; //LF
    in[INPUT_BUFFER -1] = '\0';

    while(ret != 0 && pos == 0)
    {
        ret = rb->kbd_input(in, INPUT_BUFFER -1);
        rb->lcd_clear_display();
        rb->lcd_update();
    }

    if ((in[0] >= '0') && (in[0] <= '9') && pos == 0)
        out = rb->atoi(in);
    else
    {
        out = in[pos++];
        for (int i = 0; i < INPUT_BUFFER -1 ; i++)
            if (in[i] == '\0')
                in[i] = '\n'; //LF

        if (out == '\n' || out == '\0' || pos >= 4)
        {
            for (pos = 0; pos < INPUT_BUFFER - 2 ; pos++)
                in[pos] = '\n'; //LF
            in[0] = '\0';
            pos = 0;
        }
    }
    return out;
}
int32_t save_bf(char * file_name, int32_t insptr, int32_t dataptr, uint16_t stkptr)
{
    int32_t bytes_write = 0;
    int fd_sv = rb->open(file_name, O_RDWR | O_TRUNC | O_CREAT, 0777);
    if (!fd_sv)
    {
        rb->splash(HZ * 5, "Error Saving");
        return 0;
    }
    bytes_write += rb->write(fd_sv, &insptr, sizeof(insptr));
    bytes_write += rb->write(fd_sv, &dataptr, sizeof(dataptr));
    bytes_write += rb->write(fd_sv, &stkptr, sizeof(stkptr));
    bytes_write += rb->write(fd_sv, data, DATA_SIZE);
    bytes_write += rb->write(fd_sv, stack, STACK_SIZE);
    rb->close(fd_sv);

    return bytes_write;
}
int32_t restore_bf(char * file_name, int32_t* insptr, int32_t* dataptr, uint16_t* stkptr)
{
    rb->splash(HZ * 5, "Restoring");
    uint32_t bytes_read = 0;
    uint32_t bytes_expected = (DATA_SIZE + STACK_SIZE +
                                sizeof(int32_t) + sizeof(int32_t) + sizeof(uint16_t));
    int fd_sv = rb->open(file_name, O_RDONLY);
    if (!fd_sv)
    {
        rb->splash(HZ * 5, "Error restoring");
        return 0;
    }
    bytes_read += rb->read(fd_sv, insptr, sizeof(int32_t));
    bytes_read += rb->read(fd_sv, dataptr, sizeof(int32_t));
    bytes_read += rb->read(fd_sv, stkptr, sizeof(uint16_t));
    bytes_read += rb->read(fd_sv, data, DATA_SIZE);
    bytes_read += rb->read(fd_sv, stack, STACK_SIZE);
    rb->close(fd_sv);
    if (bytes_read != bytes_expected)
    {
        rb->splashf (HZ * 5, "Error restoring %lu %lu ",
                             (unsigned long) bytes_read,
                             (unsigned long) bytes_expected);
        *insptr = 0;
        *dataptr = 0;
        *stkptr = 0;
        /*data and stack need to be set to 0 as well..*/
        return 0;
    }
    return bytes_read;
}

static inline int buf_load_bf(int fd, int32_t insptr, char* buf, unsigned int buf_sz)
{
    static int ret_err = SUCCESS;
    rb->lseek(fd, insptr, SEEK_SET);
    if (rb->read(fd, buf, buf_sz) <= 0)
        ret_err = ENDOFFILE;
    return ret_err;
}
static inline void fast_op_load(int32_t insptr, uint32_t * fast_op)
{
    rb->memcpy(fast_op, &program[insptr], sizeof(*fast_op));
}

int execute_bf(int fd)
{
    int ret_err = SUCCESS;
    off_t    file_sz = rb->filesize(fd);
    bool     prompt = true;
    int      bf_nop = 0;
    int32_t  insptr = 0;
    int32_t  buf_pos = 0;
    int32_t  dataptr = 0;
    uint16_t stkptr = 0;
    int32_t  bytes_read = 0;
    uint32_t fast_op = 0;
    if (rb->file_exists("/test.bfs"))
    {
        const struct text_message prompt =
            { (const char*[]) {"Restore?", "/test.bfs"}, 2};
        enum yesno_res response = rb->gui_syncyesno_run(&prompt, NULL, NULL);
        if(response != YESNO_NO)
            restore_bf("/test.bfs", &insptr, &dataptr, &stkptr);
    }

    buf_pos = insptr;
    ret_err |= buf_load_bf(fd, insptr, program, PROG_BUFFER);
    output(true, "\0"); //New Screen
    rb->lcd_clear_display();

    while (ret_err == SUCCESS)
    {
        if (button_prompt(false, 0,"") == PLA_EXIT)
            ret_err |= USER_EXIT;

        switch (program[insptr-buf_pos]) {
            case BF_INC_DP:
                if (!bf_nop && ++dataptr >= DATA_SIZE)
                        ret_err |= DATA_OVFL;
                break;

            case OP_INC_DP:
               /* load several bytes into fast_op use that to add or subtract */
                    fast_op_load(insptr - buf_pos +1, &fast_op);
                    dataptr += fast_op;
                    if (dataptr >= DATA_SIZE)
                        ret_err |= DATA_OVFL;
                insptr += sizeof(fast_op);
                break;

            case BF_DEC_DP:
                if (!bf_nop && --dataptr < 0)
                        ret_err |= DATA_UNFL;
                break;

            case OP_DEC_DP:
                fast_op_load(insptr - buf_pos +1, &fast_op);
                dataptr -= fast_op;
                if (dataptr < 0)
                    ret_err |= DATA_UNFL;

                insptr += sizeof(fast_op);
                break;

            case BF_INC_VAL:
                if (!bf_nop)
                    data[dataptr]++;
                break;

            case OP_INC_VAL:
                fast_op_load(insptr - buf_pos +1, &fast_op);
                data[dataptr] += fast_op;
                insptr += sizeof(fast_op);
                break;

            case BF_DEC_VAL:
                if (!bf_nop)
                    data[dataptr]--;
                break;

            case OP_DEC_VAL:
                fast_op_load(insptr - buf_pos +1, &fast_op);
                data[dataptr] -= fast_op;
                insptr += sizeof(fast_op);
                break;

            case OP_ZERO_VAL:
                data[dataptr] = 0;
                break;

            case OP_ONE_VAL:
                data[dataptr] = 1;
                break;

            case BF_OUTPUT:
                if (!bf_nop)
                {
#ifdef BF_EXEC_TIME_PROFILE
                    if (e_time == 0)
                        e_time = *rb->current_tick;//for profiling
#endif
                    if (!output    (!prompt, "%c",data[dataptr]))
                        ret_err |= USER_EXIT;
                    prompt = true;
                }
                    break;

            case BF_INPUT:
                if (!bf_nop)
                {
#ifdef BF_EXEC_TIME_PROFILE
                    if (e_time > 0)
                    {// for profiling execution speed
                        e_time = (*rb->current_tick-e_time);
                        rb->splashf(HZ, "%u", (unsigned int) e_time);
                    }
                    e_time = 0;
#endif
                    output(false, "%c",'\n'); //dump buffer
                    if (button_prompt(prompt, PLA_SELECT, prompt_continue) == PLA_EXIT)
                    {
                        ret_err |= USER_EXIT;
                        break;
                    }
                    output(true, "\0"); //New Screen
                    data[dataptr] = input();

                    prompt = false;

                }
                break;
            case OP_LOOP_ZERO:
                /* In fast_op loops the offset is recorded just after*/
                if (data[dataptr] == 0)
                {
                    fast_op_load(insptr - buf_pos +1, &fast_op);
                    insptr = fast_op;
                    break;
                }
                insptr += sizeof(fast_op);
                break;
            case OP_LOOP_NZERO:
                if (data[dataptr] != 0)
                {
                    fast_op_load(insptr - buf_pos +1, &fast_op);
                    insptr = fast_op;
                    if (insptr-buf_pos < 0)
                    {
                        ret_err |= buf_load_bf(fd, insptr, program, PROG_BUFFER);
                        buf_pos = insptr;
                    }
                    continue;
                }
                insptr += sizeof(fast_op);

                break;

            case BF_JFZ:
                if (bf_nop)
                    bf_nop++;
                else if ((data[dataptr]==0))
                    bf_nop = 1;
                else// if (bf_nop == 0)
                {
                    if (STACK_FULL())
                        ret_err |= STACK_OVFL;
                    else
                        STACK_PUSH(insptr);
                }
                break;

            case BF_JBNZ:
                if (bf_nop == 0)
                {
                    if (STACK_EMPTY())
                        ret_err |= STACK_UNFL;
                    else if (data[dataptr])
                    {
                        /*insptr_jmp = STACK_PEEK() + 1;
                        if (insptr-insptr_jmp == 1 &&
                            program[insptr-buf_pos-1] == BF_DEC_VAL) //[-]
                        {
                            data[dataptr] = 0; //short circuit for 0 data
                            STACK_DISCARD(); //discard address
                            break;
                        }
                        insptr = insptr_jmp;
                        */
                        insptr = STACK_PEEK() + 1; /* jump one past [ */
                        if (insptr-buf_pos < 0)
                        {
                            ret_err |= buf_load_bf(fd, insptr, program, PROG_BUFFER);
                            buf_pos = insptr;
                        }
                        continue;
                    }
                    else
                        STACK_DISCARD(); //discard address
                }
                else //if (bf_nop)
                    bf_nop--;
                break;

            case 0xFF://EOF:
                ret_err |= ENDOFFILE;

            default:
                break;
        }
        //insptr++;
        if (++insptr - buf_pos >= PROG_BUFFER - (signed) sizeof(fast_op) - 2)
        {
            ret_err |= buf_load_bf(fd, insptr, program, PROG_BUFFER);
            buf_pos = insptr;
        }
        if (insptr >= file_sz)
            ret_err |= INS_OVFL;
    }
    if ((ret_err || !STACK_EMPTY() || insptr != file_sz) &&
       (ret_err & USER_EXIT) != USER_EXIT)
    {
       rb->splashf(HZ * 5, "ip: %li dp: %li ins:(%i) %c ; bytes: %lu",
                           (signed long) insptr-1, (signed long) dataptr,
                           program[insptr-buf_pos-1], program[insptr-buf_pos-1],
                           (unsigned long) bytes_read);
        if (ret_err == SUCCESS)
        ret_err = ERROR;
    }
    else if ((ret_err & USER_EXIT) == USER_EXIT)
    {
        const struct text_message prompt =
            { (const char*[]) {"Save?", "/test.bfs"}, 2};
        enum yesno_res response = rb->gui_syncyesno_run(&prompt, NULL, NULL);
        if(response != YESNO_NO)
            save_bf("/test.bfs", insptr-1, dataptr, stkptr);
    }

    return ret_err;
}

int optimize_bf(const void* file, int fd)
{
    const char data_z = OP_ZERO_VAL;
    const char data_o = OP_ONE_VAL;
    char file_name[64]={'\0'};
    char ins_op = 0;
    unsigned char ins = 0, ins_new = 1;
    int32_t  insptr = 0;
    uint16_t stkptr = 0;
    uint16_t bkt_open_ct = 0, bkt_close_ct = 0;

    int ret_err = SUCCESS;
    int fd_op;
    union ins_data_u
    {
        uint32_t fast_op;
        uint8_t  cbuf[sizeof(uint16_t)];
    }ins_data;
    uint32_t bytes_write = 0;
    int bytes_read = 0;
    rb->snprintf(file_name, sizeof(file_name), "%sop%c" , (const char *) file, '\0');
    fd_op = rb->open(file_name, O_RDWR | O_TRUNC | O_CREAT, 0777);

    bytes_write = rb->write(fd_op, op_header, sizeof(op_header));
    bytes_read = rb->read(fd, &ins, 1);

    output(true, "Optimizing Source %c",'\n'); //New screen + dump buffer NOW
    output(false, "%s %c",file_name, '\n'); //dump buffer NOW
    while (ret_err == SUCCESS && bytes_read > 0)
    {
        if (button_prompt(false, 0,"") == PLA_EXIT)
            ret_err |= USER_EXIT;

        if (ins_new == 0)
            bytes_read = rb->read(fd, &ins, 1);
        else
            ins_new = 0;
        switch (ins)
        {
            case BF_INC_DP:
                ins_op = OP_INC_DP;
                break;
            case BF_DEC_DP:
                ins_op = OP_DEC_DP;
                break;
            case BF_INC_VAL:
                ins_op = OP_INC_VAL;
                break;
            case BF_DEC_VAL:
                ins_op = OP_DEC_VAL;
                break;
            case BF_JFZ://[ /* misses optimization of data_z on CRLF boundries.*/
                bytes_read = rb->read(fd, &ins_new, 1);
                if (bytes_read > 0 && ins_new == BF_DEC_VAL) //-
                {
                    bytes_read += rb->read(fd, &ins_new, 1);
                    if (bytes_read > 1 && ins_new == BF_JBNZ) //]
                    {
                        bytes_read = rb->read(fd, &ins_new, 1);
                        if (bytes_read > 0 && ins_new == BF_INC_VAL)//+
                        {
                            bytes_write += rb->write(fd_op, &data_o, 1);
                            ins_new = 0;
                            continue;
                        }
                        bytes_write += rb->write(fd_op, &data_z, 1);
                        ins = ins_new;
                        continue;
                    }
                }
                rb->lseek(fd, -bytes_read, SEEK_CUR);
                /* if the other optimizations didn't happen we will get to
                   this point, here we push the offset of the '[' onto a stack
                  and pop it at the fast_op variable after the closing brace ']'
                  we add one to it so when it is used it actually points at the
                  very next ins after the fast_op variable. THE '[' bracket
                  gets a value of one less than the actual ins since we
                  add one and check the buffer position in the loop
                  >>>>{0015->}0009++
                */
                ins_op = OP_LOOP_ZERO;
                bkt_open_ct++;
                if (STACK_FULL())
                    ret_err |= STACK_OVFL;
                else
                    STACK_PUSH(bytes_write + sizeof(ins_data.fast_op) + 1);
                case BF_JBNZ:
                ins_data.fast_op = -1;
                if (ins_op != OP_LOOP_ZERO)
                {
                    ins_op = OP_LOOP_NZERO;
                    bkt_close_ct++;
                    if (STACK_EMPTY())
                        ret_err |= STACK_UNFL;
                    else
                        ins_data.fast_op = STACK_POP();
                }

                bytes_write += rb->write(fd_op, &ins_op, 1);
                bytes_write += rb->write(fd_op,
                                         ins_data.cbuf, sizeof(ins_data.fast_op));
                ins_new = 0;
                ins_op = 0;
                continue;
            case BF_OUTPUT:
            case BF_INPUT:
                ins_new = 0;
                bytes_write += rb->write(fd_op, &ins, 1);
                continue;
            case 0xFF://EOF:
                ret_err = ENDOFFILE;
                continue;
            default:
                continue;
        }
        ins_data.fast_op = 0;
        if (bytes_read > 0 && ins != 0xFF)//EOF)
		{
            bytes_read = rb->read(fd, &ins_new, 1);
		    while (bytes_read > 0 && ins_new != 0xFF//EOF
                                  && ins == ins_new)
            {
                ins_data.fast_op++;
                bytes_read = rb->read(fd, &ins_new, 1);
            }
            if (ins_data.fast_op == 0)
			    ins_op = ins;
            ins = ins_new;
		}
        if(ins_data.fast_op > 0)
        {
            ins_data.fast_op++;
            bytes_write += rb->write(fd_op, &ins_op, 1);
            bytes_write += rb->write(fd_op,
                                     ins_data.cbuf, sizeof(ins_data.fast_op));
        }
        else
            bytes_write += rb->write(fd_op, &ins_op, 1);
    }

    rb->close(fd);

    if (ret_err == SUCCESS) /*fill jump offsets for loops*/
    {
        output(false, "Calculating loop offsets. %c",'\n');
        output(false, "[ %lu, ] %lu %c", (unsigned long) bkt_open_ct,
                                         (unsigned long) bkt_close_ct,'\n');
        insptr = 0;
        rb->lseek(fd_op, 0, SEEK_SET);
        ins_new = 1;
        bytes_read = rb->read(fd_op, &ins, 1);
        while (ret_err == SUCCESS && bytes_read > 0)
        {
            if (button_prompt(false, 0,"") == PLA_EXIT)
                ret_err |= USER_EXIT;

            if (ins_new == 0)
            {
                bytes_read = rb->read(fd_op, &ins, 1);
                insptr++;
            }
            ins_new = 0;
            //rb->splashf(HZ, "%c, %i", ins, insptr);
            switch(ins)
            {
                case OP_LOOP_NZERO:
                    insptr += sizeof(ins_data.fast_op);
                    bytes_read = rb->read(fd_op,
                                          &ins_data.cbuf, sizeof(ins_data.fast_op));
                    if (bytes_read == sizeof(ins_data.fast_op))
                    {
                        rb->lseek(fd_op,
                                  ins_data.fast_op - sizeof(ins_data.fast_op),
                                  SEEK_SET);

                        ins_data.fast_op = insptr;
                        rb->write(fd_op, ins_data.cbuf, sizeof(ins_data.fast_op));
                        rb->lseek(fd_op, insptr + 1, SEEK_SET);//ret to prev position
                    }
                    break;
                /*Skip these, have to jump past them since their fast_op variable
                  might contain 123 or 125('[]')*/
                case OP_LOOP_ZERO:
                case OP_INC_DP:
                case OP_DEC_DP:
                case OP_INC_VAL:
                case OP_DEC_VAL:
                    insptr += sizeof(ins_data.fast_op);
                    rb->lseek(fd_op, insptr+1, SEEK_SET);
                    break;

                case 0xFF://EOF:
                    rb->splashf(HZ * 10, "EOF %lu", (unsigned long) insptr - 1);
                    ret_err |= ENDOFFILE;
                    continue;
                default:
                    continue;
            }
        }

    }
    rb->close(fd_op);
    if (ret_err == SUCCESS)
    {
        fd_op = rb->open(file_name, O_RDONLY);
        rb->lseek(fd_op, 0, SEEK_SET);
        rb->lcd_clear_display();
        ret_err = execute_bf(fd_op);
        rb->close(fd_op);
    }
return ret_err;
}

enum plugin_status plugin_start(const void* file)
{
    int status;
    int fd;
    char f_ret = '\0';
    bool optimized = true;
    if (!file)
        return PLUGIN_ERROR;

    fd = rb->open(file, O_RDONLY);
    if (!fd)
        return PLUGIN_ERROR;
    for( int i = 0; i<8 ; i++)
    {
        rb->read(fd, &f_ret, 1);
        if (f_ret != op_header[i])
        {
            optimized =false;
            break;
        }
    }
    rb->lseek(fd, 0, SEEK_SET);

    rb->lcd_clear_display();
    if(!optimized)
    {
        const struct text_message prompt =
            { (const char*[]) {"Optimize Source?", "This will take a moment"}, 2};
        enum yesno_res response = rb->gui_syncyesno_run(&prompt, NULL, NULL);
        if(response == YESNO_NO)
            optimized = true;
        else
            optimized = false;
}


    if (!optimized)
        status = optimize_bf(file, fd);
    else
    {
        status = execute_bf(fd);
        rb->close(fd);
    }
    rb->lcd_clear_display();

    if (status & ERROR)
        output(false, "\nError executing");
    if (status & STACK_UNFL )
        output(false, "\nError stack underflow");
    else if (status & STACK_OVFL )
        output(false, "\nError stack overflow");
    if (status & DATA_UNFL )
        output(false, "\nError data underflow");
    else if (status & DATA_OVFL )
        output(false, "\nError data overflow");
    if (status & ENDOFFILE )
        output(false, "\nError end of file");
    if (status & INS_OVFL )
        output(false, "\nError ins overflow");

    button_prompt(true, PLA_SELECT, "Any key to Exit                         ");

    return PLUGIN_OK;
}

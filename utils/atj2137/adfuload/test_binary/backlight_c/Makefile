
TARGET	= test_bl_c

TOOLCHAIN	= mipsel-elf-

CC		= $(TOOLCHAIN)gcc
CPP		= $(TOOLCHAIN)cpp
LD		= $(TOOLCHAIN)gcc
AS		= $(TOOLCHAIN)as
OBJCOPY	= $(TOOLCHAIN)objcopy
OBJDUMP	= $(TOOLCHAIN)objdump

CFLAGS	= -Wundef -march=mips32r2 -nostdlib -Os -c

OBJS	= crt0.o test_bl.o
LDSCRIPT= test_bl.lds

LDFLAGS	= -Wundef -march=mips32r2 -T$(LDSCRIPT) -nostartfiles  \
	-nostdlib -Xlinker -Map=$(TARGET).map

all		: $(TARGET).bin
		ls -ls $(TARGET).bin

%.o		: %.c
		$(CC) $(CPPFLAGS) $(CFLAGS) $(INCDIRS) $< -o $@

%.o		: %.S
		$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf : $(OBJS)
		$(LD) $(LDFLAGS) $(OBJS) $(LIBDIRS) $(LIBS) -o $(TARGET).elf

$(TARGET).bin : $(TARGET).elf
		$(OBJCOPY) -O binary $(TARGET).elf $(TARGET).bin

clean	:
		rm -f $(OBJS)
		rm -f $(TARGET).elf
		rm -f $(TARGET).bin
		rm -f $(TARGET).map

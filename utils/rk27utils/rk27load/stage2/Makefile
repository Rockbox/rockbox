
TARGET	= stage2

TOOLCHAIN	= arm-elf-eabi-

CC		= $(TOOLCHAIN)gcc
CPP		= $(TOOLCHAIN)cpp
LD		= $(TOOLCHAIN)gcc
AS		= $(TOOLCHAIN)as
OBJCOPY	= $(TOOLCHAIN)objcopy
OBJDUMP	= $(TOOLCHAIN)objdump

CFLAGS	= -Wundef -marm -march=armv5te -nostdlib -mfpu=fpa -O0 -c
#ASFLAGS	= -mcpu=arm926ej-s

OBJS	= crt0.o main.o irq.o
LDSCRIPT= stage2.lds

#LIBDIRS	= -L../arm/lib/gcc/arm-elf/4.1.0/ -L../lib
#LIBS	= -lgcc
LIBS    =
LDFLAGS	= -Wundef -marm -march=armv5te -T$(LDSCRIPT) -nostartfiles  \
	-mfpu=fpa -nostdlib -Xlinker -Map=$(TARGET).map

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

dasm	: $(TARGET).bin
		$(OBJDUMP) -m arm -D $(TARGET).elf | cat > $(TARGET).asm

clean	:
		rm -f $(OBJS)
		rm -f $(TARGET).elf
		rm -f $(TARGET).bin
		rm -f $(TARGET).asm
		rm -f $(TARGET).map

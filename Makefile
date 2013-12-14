#SOURCES_ASM := $(wildcard *.S)
#SOURCES_C   := $(wildcard *.c)
 
#OBJS        := $(patsubst %.S,%.o,$(SOURCES_ASM))
#OBJS        += $(patsubst %.c,%.o,$(SOURCES_C))

MACH=at91
SOC=sam3x

BCM2835_LD=link-arm-eabi.ld
SAM3X_LD=sam3x.ld
SAM3X8_SRAM_LD=sram.ld
LD_SCRIPT=$(SAM3X8_SRAM_LD)
 
DEPENDFLAGS := -MD -MP
INCLUDES    := -I$(TOOLCHAIN_DIR)arm-unknown-eabi/include
INCLUDES 	+= -I$(KERNEL_BASE)/include
INCLUDES 	+= -I$(KERNEL_BASE)/boards
BASEFLAGS   := -O2 -fpic -pedantic -pedantic-errors -nostdlib
BASEFLAGS   += -nostartfiles -ffreestanding -nodefaultlibs
BASEFLAGS   += -fno-builtin -fomit-frame-pointer -mcpu=arm1176jzf-s
#WARNFLAGS   := -Wall -Wextra -Wshadow -Wcast-align -Wwrite-strings
WARNFLAGS   := -Wextra -Wshadow -Wcast-align -Wwrite-strings
WARNFLAGS   += -Wredundant-decls -Winline
WARNFLAGS   += -Wno-attributes -Wno-deprecated-declarations
WARNFLAGS   += -Wno-div-by-zero -Wno-endif-labels -Wfloat-equal
WARNFLAGS   += -Wformat=2 -Wno-format-extra-args -Winit-self
WARNFLAGS   += -Winvalid-pch -Wmissing-format-attribute
WARNFLAGS   += -Wmissing-include-dirs -Wno-multichar
WARNFLAGS   += -Wredundant-decls -Wshadow
WARNFLAGS   += -Wno-sign-compare -Wswitch -Wsystem-headers -Wundef
WARNFLAGS   += -Wno-pragmas -Wno-unused-but-set-parameter
WARNFLAGS   += -Wno-unused-but-set-variable -Wno-unused-result
WARNFLAGS   += -Wwrite-strings -Wdisabled-optimization -Wpointer-arith
WARNFLAGS   += -Werror
ASFLAGS     := $(INCLUDES) $(DEPENDFLAGS) -D__ASSEMBLY__
CFLAGS      := $(INCLUDES) $(DEPENDFLAGS) $(BASEFLAGS) $(WARNFLAGS)
CFLAGS      += -std=gnu99

OBJS	:= 	asm/head.o \
			boards/mach-$(MACH)/$(SOC).o \
			boards/mach-$(MACH)/uart-$(SOC).o \
			boot/boot-$(SOC).o \
			drivers/uart.o \
			kernel/main.o \
			kernel/interrupts.o \
			kernel/scheduler.o \
			utils/io.o

config:
	@@echo "CP mach-$(MACH)/board-$(SOC).h -> board.h"
	@cp boards/mach-$(MACH)/board-$(SOC).h boards/board.h
 
all: kernel.img
 
include $(wildcard *.d)
 
kernel.elf: $(OBJS) 
	@@echo "LD " $<
	@$(CROSS_COMPILE)ld $(OBJS) -T$(LD_SCRIPT) -o $@
 
kernel.img: kernel.elf
	@@echo "OBJCOPY " $<
	@$(CROSS_COMPILE)objcopy kernel.elf -O binary kernel.img
 
clean:
	$(RM) -f $(OBJS) kernel.elf kernel.img
	$(RM) boards/board.h
 
dist-clean: clean
	$(RM) -f *.d
 
%.o: %.c
	@@echo "CC " $<
	@$(CROSS_COMPILE)gcc $(CFLAGS) -c $< -o $@
 
%.o: %.S
	@@echo "CC " $<
	@$(CROSS_COMPILE)gcc $(ASFLAGS) -c $< -o $@

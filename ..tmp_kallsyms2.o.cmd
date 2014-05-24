cmd_.tmp_kallsyms2.o := /home/nsdcars5/areskernel/scripts/gcc-wrapper.py /home/nsdcars5/android-toolchain-eabi/bin/arm-eabi-gcc -Wp,-MD,./..tmp_kallsyms2.o.d -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables  -D__LINUX_ARM_ARCH__=7 -march=armv7-a  -include asm/unified.h -msoft-float -gdwarf-2 -w   -nostdinc -isystem /home/nsdcars5/android-toolchain-eabi/bin/../lib/gcc/arm-eabi/4.6.2/include -I/home/nsdcars5/areskernel/arch/arm/include -Iarch/arm/include/generated -Iinclude  -include /home/nsdcars5/areskernel/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-msm/include    -c -o .tmp_kallsyms2.o .tmp_kallsyms2.S

source_.tmp_kallsyms2.o := .tmp_kallsyms2.S

deps_.tmp_kallsyms2.o := \
  /home/nsdcars5/areskernel/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
    $(wildcard include/config/thumb2/kernel.h) \
  /home/nsdcars5/areskernel/arch/arm/include/asm/types.h \
  include/asm-generic/int-ll64.h \
  arch/arm/include/generated/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
    $(wildcard include/config/64bit.h) \

.tmp_kallsyms2.o: $(deps_.tmp_kallsyms2.o)

$(deps_.tmp_kallsyms2.o):

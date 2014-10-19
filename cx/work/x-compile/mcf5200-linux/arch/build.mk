#
#	build.mk -- common definitions for building user apps.
#

.EXPORT_ALL_VARIABLES:

ROOTDIR = $(CM5307_ROOT)
TOOLS = $(ROOTDIR)/tools

CC      = $(TOOLS)/m68k-elf-gcc
AR      = $(TOOLS)/m68k-elf-ar
LD      = $(TOOLS)/m68k-elf-ld -r
LDGDB   = $(TOOLS)/m68k-elf-ld
OBJCOPY = $(TOOLS)/m68k-elf-objcopy
RANLIB  = $(TOOLS)/m68k-elf-ranlib
ELF2FLT = $(TOOLS)/elf2flt
FLTFLAGS = -s 4096

GCC_EXEC_PREFIX = $(TOOLS)/m68k-elf-

LIBC    = $(ROOTDIR)/lib/libc.a
LIBM    = $(ROOTDIR)/lib/libmf.a
LIBGCC  = $(TOOLS)/gcc-lib-cf/libgcc.a

INCLIBC = -I$(ROOTDIR)/include
INCGCC  = -I$(TOOLS)/gcc-include

ARCH    = -m5200 -Wa,-m5200 -DCONFIG_COLDFIRE
DEFS    = -D__linux__
INCS    = $(INCGCC) $(INCLIBC)
CCFLAGS = -c -O2 -msoft-float -fno-builtin
CFLAGS  = $(ARCH) $(DEFS) $(CCFLAGS) $(INCS)

LDSCRIPT = $(ROOTDIR)/arch/user.ld
STARTUP  = $(ROOTDIR)/lib/crt0.o
LDFLAGS  = -T $(LDSCRIPT) $(STARTUP)
LDLIBS   = --start-group $(LIBC) $(LIBGCC) $(LIBM) --end-group
CONVERT  = $(ELF2FLT) $(FLTFLAGS) -o $@ $@.elf


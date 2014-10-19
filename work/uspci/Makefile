.PHONY:	all clean kernel_module utils

all:	kernel_module utils

kernel_module utils:
	make -C $@

clean:
	set -e; \
	make -C kernel_module clean; \
	make -C utils clean

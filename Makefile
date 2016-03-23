
# Demo makefile for instrument.h module.

HEADERS = $(wildcard *.h)

SOURCES = $(wildcard *.c)

OBJS = $(patsubst %.c,%.o,$(SOURCES))

# We use some GNU libc extension functions, and this serves as a example
# of how to pass the same cpp flags to cflow as well as the compiler.
CPPFLAGS = -D_GNU_SOURCE

# Uncomment to enable incremental linking with gold.  It didn't work for me.
# FIXME: try again once gold is better
#INCR_LDFLAGS = -fuse-ld=gold -Wl,--incremental

# Compilation of C files.  In real life client and library files are unlikely
# to be compiled the same way, and automatic dependency tracking is often
# used to computer .c->.h dependencies.
$(OBJS): %.o: %.c $(HEADERS) Makefile
	# See the comments in instrument.h for the reasons for these options
	# and for the double build.
	gcc -c $(CPPFLAGS) -Wall -Wextra -Werror -g -O2 -fPIC $< -o $@
	gcc -c $(CPPFLAGS) -Wall -Wextra -Werror -g -O0 -fPIC $< -o $@

# Shared library for demonstration purposes
libdemo_shared_lib.so: demo_shared_lib.o
	# See the Program Library Howto for a real life shared lib/DLL setup
	gcc -Wl,-soname,$@ -fPIC -rdynamic -shared $< -o $@

# FIXME: maybe use -fPIE for the executables if -fPIC won't work reliable?

# Executable program exercising instrument.h
instrument_test: instrument_test.o instrument.o libdemo_shared_lib.so
	# See the Program Library Howto for a real life shared lib/DLL setup
	gcc -Wl,-rpath,`pwd` -fPIC $+ -ldl -o $@

.PHONY: run_instrument_test
run_instrument_test: instrument_test
	./$< || echo Recipe succeeding anyway because ./$< is expected to fail

# If you'r interested in using cflow or global check this, otherwise ignore:
-include cflow_and_global.mk

.PHONY: clean
clean:
	rm -rf *.o *.so* instrument_test core $(CFLOW_AND_GLOBAL_CLEANFILES)

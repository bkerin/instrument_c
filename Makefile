
# Demo makefile for instrument.h module.

HEADERS = $(wildcard *.h)

SOURCES = $(wildcard *.c)

OBJS = $(patsubst %.c,%.o,$(SOURCES))

# We use some GNU libc extension functions, and this serves as a good
# example of how to pass cpp flags to cflow.
CPPFLAGS = -D_GNU_SOURCE

# Compilation of C files.  In real life static pattern rules are better,
# client and library files are unlikely to be compiled the same way, and
# automatic dependency tracking is often used to computer .c->.h dependencies.
%.o: %.c $(HEADERS) Makefile
	# See the comments in instrument.h for the reasons for these options
	# and for the double build.
	gcc -c $(CPPFLAGS) -Wall -Wextra -Werror -g -O2 -fpic $< -o $@
	gcc -c $(CPPFLAGS) -Wall -Wextra -Werror -g -O0 -fpic $< -o $@

# Shared library for demonstration purposes
libdemo_shared_lib.so: demo_shared_lib.o
	# See the Program Library Howto for a real life shared lib/DLL setup
	gcc -rdynamic -shared -Wl,-soname,$@ $< -o $@

# Executable program exercising instrument.h
instrument_test: instrument_test.o instrument.o libdemo_shared_lib.so
	# See the Program Library Howto for a real life shared lib/DLL setup
	gcc $+ -ldl -Wl,-rpath,`pwd` -o $@

.PHONY: run_instrument_test
run_instrument_test: instrument_test
	./$< || echo Recipe succeeding anyway because ./$< is expected to fail


.PHONY: clean
clean:
	rm -f *.o *.so* instrument_test core

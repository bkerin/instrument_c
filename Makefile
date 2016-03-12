# FIXME: make sure to update the memmap program with the better version
# of client.c.  Actually it should just become a pointer to this project

# Demo makefile for instrument.h module.

# Compilation of C files.  In real life static pattern rules are better.
%.o: %.c Makefile
	# See the comments in instrument.h for info about the options used here
	gcc -c -Wall -Wextra -Werror -g -O2 -fpic $< -o $@
	gcc -c -Wall -Wextra -Werror -g -O0 -fpic $< -o $@

# Shared library for demonstration purposes
libtest.so: test.o
	# See the Program Library Howto for a real life shared lib/DLL setup
	gcc -rdynamic -shared -Wl,-soname,$@ $< -o $@
        
# Executable program exercising instrument.h
instrument_test: instrument_test.o instrument.o libtest.so
	# See the Program Library Howto for a real life shared lib/DLL setup
	gcc $+ -ldl -Wl,-rpath,`pwd` -o $@

.PHONY: run_instrument_test
run_instrument_test: instrument_test
	./$<

.PHONY: clean
clean:
	rm -f *.o *.so* instrument_test


# Demo makefile for instrument.h module.

HEADERS = $(wildcard *.h)

SOURCES = $(wildcard *.c)

OBJS = $(patsubst %.c,%.o,$(SOURCES))

# Uncomment this to use ccache to avoid unnecessary recompilation.
# For example, this Makefile uses a conservative build-too-much strategy
# in which every object file depends on all headers and the Makefile.
# The use of ccache mostly eliminates the build time penalty of this approach.
#CCACHE = ccache

# The GNU gold linker is exciting, because incremental linking could speed
# up edit-compile-debug significantly for large projects.  Unfortunately
# it isn't done as of this writing: it only works for x86_64 targets,
# and it's output confuses the current nm and addr2line implementations.
# Uncomment this and add it to the gcc linking invocations to try gold anyway.
# FIXMELATER: enable this someday
#INCR_LDFLAGS = -fuse-ld=gold -Wl,--incremental

CC = $(CCACHE) gcc

# We use some GNU libc extension functions.  This also serves as a example
# of how to pass the same cpp flags to cflow as well as the compiler.
CPPFLAGS = -D_GNU_SOURCE

# Object files.  In real life client and library files aren't usually
# compiled in the same make recipe, and automatic dependency tracking is
# often used to compute .c->.h dependencies.
$(OBJS): %.o: %.c $(HEADERS) Makefile
	# See the comments in instrument.h for the reasons for these options
	# and for the double build.
	$(CC) -c $(CPPFLAGS) -Wall -Wextra -Werror -g -O2 -fPIC $< -o $@
	$(CC) -c $(CPPFLAGS) -Wall -Wextra -Werror -g -O0 -fPIC $< -o $@

# Shared library for demonstration purposes
libdemo_shared_lib.so: demo_shared_lib.o
	# See the Program Library Howto for a real life shared lib/DLL setup
	$(CC) -Wl,-soname,$@ -rdynamic -shared $< -o $@

# Executable program exercising the instrument.h interface
instrument_test: instrument_test.o instrument.o libdemo_shared_lib.so
	# See the Program Library Howto for a real life shared lib/DLL setup
	$(CC) -Wl,-rpath,`pwd` $+ -ldl -o $@

.PHONY: run_instrument_test
run_instrument_test: instrument_test
	./$< || echo Recipe succeeding anyway because ./$< is expected to fail

# If you'r interested in using cflow or global check this, otherwise ignore:
-include cflow_and_global.mk

.PHONY: clean
clean:
	rm -rf *.o *.so* instrument_test core $(CFLOW_AND_GLOBAL_CLEANFILES)

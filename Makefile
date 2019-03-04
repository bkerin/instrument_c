
include sanity.mk

HEADERS = $(wildcard *.h)

SOURCES = $(wildcard *.c)

OBJS = $(patsubst %.c,%.o,$(SOURCES))

# Uncomment this to use ccache to avoid unnecessary recompilation.
# For example, this Makefile uses a conservative build-too-much strategy
# in which every object file depends on all headers and the Makefile.
# The use of ccache mostly eliminates the build time penalty of this approach.
#CCACHE = ccache

# The GNU gold linker is exciting, because incremental linking could speed
# up edit-compile-debug significantly for large projects.  Unfortunately it
# isn't done as of this writing: it only works for x86_64 targets, and
# its output confuses the current nm and addr2line implementations (they
# produce incorrect DWARF version warnings and may have other issues).
# Uncomment this to try gold anyway.  Note that --incremental is not really
# intended to be used for production releases, only for development, so
# you'll want to arrange for your final production build to ommit it.
#INCR_LDFLAGS = -fuse-ld=gold -Wl,--incremental

CC = $(CCACHE) gcc

# We use some GNU libc extension functions.  This also serves as a example
# of how to pass the same cpp flags to cflow as well as the compiler.
CPPFLAGS = -D_GNU_SOURCE

# FIXME: make sure everything builds with as many warns on as possible

CFLAGS = -Wall -Wextra -Werror -Wformat-signedness -Wpointer-arith -g -fPIC -O0

# Only Because we want to test the optional format_free_print_pt_extensions.h
CPPFLAGS += -DHAVE_FORMAT_FREE_PRINT_PT_EXTENSIONS_H

$(OBJS): %.o: %.c $(HEADERS) Makefile
	# See the comments in instrument.h for the reasons for these options
	# and for the double build.
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -O2 $< -o $@   # To trigger errs/warns
	$(CC) -c $(CPPFLAGS) $(CFLAGS)     $< -o $@   # To ensure no inline

format_free_print_test: format_free_print_test.o
	$(CC) $(INCR_LDFLAGS) $+ -o $@

.PHONY: format_free_print_test_run
run_format_free_print_test: format_free_print_test
	./$<

libdemo_shared_lib.so: demo_shared_lib.o
	# See the Program Library Howto for a real life shared lib/DLL setup
	$(CC) $(INCR_LDFLAGS) -Wl,-soname,$@ -rdynamic -shared $< -o $@

instrument_test: instrument_test.o instrument.o libdemo_shared_lib.so
	# See the Program Library Howto for a real life shared lib/DLL setup
	$(CC) $(INCR_LDFLAGS) -Wl,-rpath,`pwd` $+ -ldl -o $@

.PHONY: run_instrument_test
run_instrument_test: instrument_test
	./$<                                                                  \
        ||                                                                    \
        (                                                                     \
          echo ;                                                              \
          echo WARNING: Recipe succeeding because ./$< is expected to fail, ; \
          echo its output is shoud be validated *manually*.                   \
        ) 1>&2

# If you're interested in using cflow or global look at this:
-include cflow_and_global.mk

# If you're interested in building and installing libinstrument.a look at this:
-include instrument_lib.mk


.PHONY: clean
clean:
	rm -rf                            \
           *.o                            \
           *.so*                          \
           format_free_print_test         \
           instrument_test                \
           $(CFLOW_AND_GLOBAL_CLEANFILES) \
           $(INSTRUMENT_LIB_CLEANFILES)   \
           core


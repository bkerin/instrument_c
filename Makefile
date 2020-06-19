
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

# Need gcc-8 (or later) to test latest -W options
# FIXME: replace this with a generic gcc and a version check somewhere so it
# can work for random people
CC = $(CCACHE) $(HOME)/opt/gcc-10.1.0/bin/gcc  

# We use some GNU libc extension functions.  This also serves as a example
# of how to pass the same cpp flags to cflow as well as the compiler.
CPPFLAGS = -D_GNU_SOURCE

# FIXME: Probably make all the quickie things like PT lowercase, because
# that's easlier to type and using upcase for macro names is pointless
# and passee.  But then again it's sort of nice to be able search out all
# these macros without matching extra variable names as much, so maybe lose
# this fixme.  ASSERT_BT also in theory.

# Source libs must build cleanly with strictest possible warning settings.
# Generally format_free_print.h and instrument.h do the excruciatingly
# correct things required to avoid these warnings, but there's one exception:
# the warning about function to object pointer casts that are activated by
# -Wpedantic are explicitly suppressed using a _Pragma on GCC.  So if you
# really are on a machine with a separate address space for functions be
# warned :).
STACK_USAGE_LIMIT = 64042   # Just for -Wstack-usage pass check
SUPPORTED_WARNING_OPTIONS =                        \
  -pedantic-errors                                 \
  $(addprefix -W,                                  \
                  aggregate-return                 \
                  aggressive-loop-optimizations    \
                  all                              \
                  alloc-zero                       \
                  alloca                           \
                  array-bounds=2                   \
                  attribute-alias                  \
                  extra                            \
                  bad-function-cast                \
                  cast-qual                        \
                  cast-align=strict                \
                  conversion                       \
                  date-time                        \
                  declaration-after-statement      \
                  disabled-optimization            \
                  duplicated-branches              \
                  duplicated-cond                  \
                  float-equal                      \
                  format=2                         \
                  format-overflow=2                \
                  format-signedness                \
                  format-truncation=2              \
                  hsa                              \
                  implicit-fallthrough=5           \
                  init-self                        \
                  inline                           \
                  invalid-pch                      \
                  jump-misses-init                 \
                  logical-op                       \
                  missing-declarations             \
                  missing-include-dirs             \
                  missing-prototypes               \
                  null-dereference                 \
                  nested-externs                   \
                  normalized=nfkc                  \
                  old-style-definition             \
                  openmp-simd                      \
                  overlength-strings               \
                  packed                           \
                  padded                           \
                  parentheses                      \
                  pedantic                         \
                  pointer-arith                    \
                  redundant-decls                  \
                  shadow=compatible-local          \
                  shadow=global                    \
                  shadow=local                     \
                  stack-protector                  \
                  stack-usage=$(STACK_USAGE_LIMIT) \
                  strict-overflow=5                \
                  strict-prototypes                \
                  stringop-overflow=4              \
                  suggest-attribute=cold           \
                  suggest-attribute=const          \
                  suggest-attribute=format         \
                  suggest-attribute=pure           \
                  suggest-attribute=noreturn       \
                  suggest-attribute=malloc         \
                  switch-bool                      \
                  switch-default                   \
                  switch-enum                      \
                  switch-unreachable               \
                  sync-nand                        \
                  trampolines                      \
                  undef                            \
                  uninitialized                    \
                  unsuffixed-float-constants       \
                  unused                           \
                  unused-macros                    \
                  unused-parameter                 \
                  unused-const-variable=2          \
                  vla                              \
                  write-strings                    )

CFLAGS = -fstack-protector $(SUPPORTED_WARNING_OPTIONS) -Werror -g -fPIC -O0

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

instrument_test: instrument_test.o libdemo_shared_lib.so
	# See the Program Library Howto for a real life shared lib/DLL setup
	$(CC) $(INCR_LDFLAGS) -Wl,-rpath,`pwd` $+ -ldl -o $@

.PHONY: run_instrument_test
run_instrument_test: instrument_test
	./$<                                                                  \
        ||                                                                    \
        (                                                                     \
          echo ;                                                              \
          echo WARNING: Recipe succeeding because ./$< is expected to fail. ; \
          echo Its output shoud be validated manually.                        \
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


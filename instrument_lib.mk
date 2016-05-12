
libinstrument.a: instrument.o
	ar rcs $@ $+

# To install somewhere different do e.g. make install PREFIX=/some/dir
PREFIX ?= /usr/local

install: libinstrument.a
	cp $< $(PREFIX)/lib
	# It's a bit weird to always install all headers whether the user built
	# with HAVE_INSTRUMENT_FORMAT_FREE_PRINT_H or not, but it simplest
	cp instrument.h instrument_format_free_print.h $(PREFIX)/include

# Like instrument_test from Makefile, but use the installed library.
# Note that this will fail unless install has been done (and the library
# ends up somewhere findable), but because that might need PREFIX and is
# normally user-executed we don't depend on it.
instrument_test_using_installed_lib: instrument_test.o libdemo_shared_lib.so
	# Verify that the include file is #include'able
	echo -e '#include "instrument.h"\n int main (void) { return 0; }' \
                >/tmp/instrument_includer_temp.c
	gcc $(CPPFLAGS) -Wall -Wextra -Werror /tmp/instrument_includer_temp.c
	# Verify that linking against the library works
	$(CC) $(INCR_LDFLAGS) -Wl,-rpath,`pwd` $+ -linstrument -ldl -o $@

# Like run_instrument_test from Makefile, but run the binary that uses the
# installed instrument library
.PHONY: test_install
test_install: instrument_test_using_installed_lib
	# We've already build the library, make sure it works at run-time
	./$< || echo Recipe succeeding anyway because ./$< is expected to fail

INSTRUMENT_LIB_CLEANFILES = libinstrument.a instrument_test_using_installed_lib

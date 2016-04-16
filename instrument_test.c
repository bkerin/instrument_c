// Exercise the interface described in instrument.h

#include <stdbool.h>
#include <stdio.h>

#include "demo_shared_lib.h"
#include "instrument.h"

void
basic_func (void);

void
basic_func (void)
{
  printf ("in function %s\n", __func__);
}

static void
static_func (void)
{
  printf ("in function %s\n", __func__);
}

static void
i_fail_with_backtrace (void)
{
  ASSERT_BT (false);
}

static int
i_return_an_int (void)
{
  return 42;
}

int
main (void)
{
  int test_int = 42;

  printf ("\n");
  
  // Announce ourselves and call the test functions to keep the compiler
  // from issuing warnings and make sure they really exist.
  printf ("I'm a client program\n");
  basic_func ();
  static_func ();
  demo_shared_lib_func ();
  printf ("\n");

  printf ("Testing instrumentation macros CP(), TV(), and maybe TS()...\n");
  CP ();
  TV (test_int, %i);
  TV (i_return_an_int (), %i);
#ifdef __GNUC__   // This one needs GNU comma-swallowing __VA_ARGS__ extension
  TS ("test_int: %i, literal_string: %s", test_int, "42 also :)");
#endif
  printf ("\n");

  printf ("Looking up name for function basic_func()...\n");
  what_func (basic_func);
  printf ("\n");

  // Note that the optimizer tends to inline static functions especially often
  printf ("Looking up name for static function static_func()...\n");
  what_func (static_func);
  printf ("\n");

  printf ("Looking up name for demo_shared_lib_func()...\n");
  what_func (demo_shared_lib_func);
  printf ("\n");

  // See the Makefile if you want to try this optional functionality.
#ifdef HAVE_INSTRUMENT_FORMAT_FREE_PRINT_H
  printf ("Trying format-free print and trace macros...\n");
  char *test_string = "I'm a test string";
  TT (test_string);
  char const *test_const_string = "I'm a test constant string";
  TT (test_const_string);
  TT ("I'm a literal string");  // Fires the char [] handler
  TT (test_int);
  TT (i_return_an_int ());
  int const test_const_int = 42;
  TT (test_const_int);
  float test_float = 42.4;
  TT (test_float);
  // Note that the stringified label used for literal floating point values
  // isn't subject to printf() rounding, but the value itself is.
  TT (42.424211111);
  long double test_long_double = 42.4242;
  TT (test_long_double);
  PT ("Plain PT() (without source locations or implicit newlines) ");
  PT ("is also available.\n");
  printf ("\n");
#endif

  // See the Makefile if you want to try this optional functionality.
#ifdef HAVE_INSTRUMENT_PT_EXTENSIONS_H
  printf ("Trying format-free trace of an extended type...\n");
  Widget test_widget;
  test_widget.name = "test_widget";
  // Normally this next call would probably go somewhere else.
  register_printf_specifier ('W', print_widget, print_widget_arginfo);
  TT (&test_widget);
  printf ("\n");
#endif

  printf ("Calling function i_fail_with_backtrace() (expecting failure)...\n");
  i_fail_with_backtrace ();

  return 0;
}

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

int
main (void)
{
  int test_int = 42;

  printf ("\n");
  
  // Announce ourselves and call the test functions to keep the compiler
  // from issuing warning and make sure they really exist.
  printf ("I'm a client program\n");
  basic_func ();
  static_func ();
  demo_shared_lib_func ();
  printf ("\n");

  printf ("Testing instrumentation macros CP(), TV(), and maybe TS()...\n");
  CP ();
  TV (test_int, %i);
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

// FIXME: remove old version once new ones verified

#ifdef HAVE_INSTRUMENT_FORMAT_FREE_PRINT_H
  printf ("Trying format-free print and trace macros...\n");
  char *test_string = "I'm a test string";
  TT (test_string);
  char const *test_const_string = "I'm a test constant string";
  TT (test_const_string);
  TT ("I'm a test literal string");  // Fires the char [] handler
  TT (test_int);
  int const test_const_int = 42;
  TT (test_const_int);
  TT (42.42);
  PT ("Plain PT() ");
  PT ("is useful for constructing single-line output.\n");
  printf ("\n");

  printf ("\n");

  printf ("Trying new format-free print and trace macros...\n");
  TT2 (test_string);
  TT2 (test_const_string);
  TT2 ("I'm a test literal string");  // Fires the char [] handler
  TT2 (test_int);
  TT2 (test_const_int);
  TT2 (42.42);
  PT2 ("Plain PT() (without source locations or trailing newlines) ");
  PT2 ("is also available.\n");
  size_t size_tester = 42;
  PT2 (size_tester);
  printf ("\n");

#endif

  printf ("Calling function i_fail_with_backtrace() (expecting failure)...\n");
  i_fail_with_backtrace ();

  return 0;
}

// Exercise the interface in instrument.h

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
  return -42;
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

  printf ("Calling function i_fail_with_backtrace() (expecting failure)...\n");
  i_fail_with_backtrace ();

  return 0;
}

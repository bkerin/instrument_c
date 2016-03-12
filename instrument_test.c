// Exercise the interface described in instrument.h

#include <stdio.h>

#include "instrument.h"
#include "test.h"

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

int
main (void)
{
  printf ("\n");

  // FIXME: rename shared_lib_func to demo_shared_lib_func?

  // Announce ourselves and call the functions being tested
  printf ("I'm a client program\n");
  basic_func ();
  static_func ();
  shared_lib_func ();
  printf ("\n");

  printf ("Looking up name for function basic_func()...\n");
  what_func (basic_func);
  printf ("\n");

  // Note that the optimizer tends to inline static functions especially often
  printf ("Looking up name for static function static_func()...\n");
  what_func (static_func);
  printf ("\n");

  printf ("Looking up name for shared library function shared_lib_func()...\n");
  what_func (shared_lib_func);
  printf ("\n");

  return 0;
}

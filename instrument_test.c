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
  char       *test_string       = "I'm a test string";
  char const *test_const_string = "I'm a test constant string";

  short int              test_short_int               = -42;
  long int               test_long_int                = -42;
  long long int          test_long_long_int           = -42;
  unsigned short int     test_unsigned_short_int      = 42;
  unsigned int           test_unsigned_int            = 42;
  unsigned long int      test_unsigned_long_int       = 42;
  unsigned long long int test_unsigned_long_long_int  = 42;
  uint8_t                test_uint8                   = 42;
  uint16_t               test_uint16                  = 42;
  uint32_t               test_uint32                  = 42;
  uint64_t               test_uint64                  = 42;
  int8_t                 test_int8                    = -42;
  int16_t                test_int16                   = -42;
  int32_t                test_int32                   = -42;
  int64_t                test_int64                   = -42;
  bool                   test_bool                    = true;
  float                  test_float                   = 42.42;
  double                 test_double                  = 42.42;
  long double            test_long_double             = 42.42;
  uint8_t                test_hex_uint8               = 0x42;
  uint16_t               test_hex_uint16              = 0xab42;
  uint32_t               test_hex_uint32              = 0x00abcd42;

  // Constant int and floating point types seem to work fine without explicit
  // mentions of the const types in the list in the PT() definition.
  int const    test_const_int    = -42;
  double const test_const_double = 42.42;

  printf ("Trying format-free print and trace macros...\n");

  TT (test_string);
  TT (test_const_string);
  TT ("I'm a literal string");  // Fires the char [] handler

  TT (test_int);
  TT (test_short_int);
  TT (test_long_int);
  TT (test_long_long_int);
  TT (test_unsigned_short_int);
  TT (test_unsigned_int);
  TT (test_unsigned_long_int);
  TT (test_unsigned_long_long_int);

  TT (test_uint8);
  TT (test_uint16);
  TT (test_uint32);
  TT (test_uint64);
  TT (test_int8);
  TT (test_int16);
  TT (test_int32);
  TT (test_int64);
  TT (INT8_C (-42));
  TT (INT16_C (-42));
  TT (INT32_C (-42));
  TT (INT64_C (-42));
  TT (UINT8_C (42));
  TT (UINT16_C (42));
  TT (UINT32_C (42));
  TT (UINT64_C (42));

  TT (test_bool);
  TT (false);
  TT (true);

  TT (test_float);
  TT (test_double);
  TT (test_long_double);
  PT ("NOTE: the stringified labels of floating point literals aren't\n");
  PT ("subject to printf() rounding, but the values themselves are:\n");
  TT (42.424211111f);
  TT (42.424211111);
  TT (42.424211111d);

  TT (test_const_int);
  TT (test_const_double);

  TT (i_return_an_int ());
  // FIXME: might be worth actually checking if all this crazy stuff works
  // for functions of all the other supported return types as well

  TTX (test_hex_uint8);
  TTX (test_hex_uint16);
  TTX (test_hex_uint32);
  
  printf ("\n");

  // Note that the stringified label used for literal floating point values
  // isn't subject to printf() rounding, but the value itself is.
  PT ("Other available variants (mostly tested indirectly by the above):\n");
  PT ("  PT (thing) -- without source locations or implicit newlines\n");
  PT ("  PL (thing) -- Like PT(), but adds a Label (e.g. var_name: 42)\n");
  PT ("  DT (thing) -- Dump Thing.  Like PL(), but also appends a newline\n");
  PT ("  TT (thing) -- Trace Thing.  Like DT() including file:line:func:\n");
  PT ("  TD (thing) -- Like TT(), but exit (EXIT_FAILURE) afterwords\n");
  PT ("  *X (thing) -- Analogous variants to the above, but only work for\n");
  PT ("                uint8_t/uint16_t/uint32_t types and output in hex\n");

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

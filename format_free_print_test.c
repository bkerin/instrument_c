// Exercise the interface in format_free_print.h

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "format_free_print.h"

static int
i_return_minus_42_as_an_int (void)
{
  // Uncomment to verify that macro argument evaluated only once:
  printf ("(this side effect output should appear only once) ");

  return -42;
}

int
main (void)
{
  char test_char = '4';

  char       *test_string               = strdup ("A string");
  char const *test_const_string         = "A constant string";
  char        test_char_array[42]       = "A literal char array";
  char const  test_char_const_array[42] = "A literal char const array";

  int                    test_int                     = -42;
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

  float                  test_float                   = 42.42F;
  /* Weirdly expressed so we can pass with -Wunsuffixed-float-constants:  */
  double                 test_double                  = ((double) 42.42L);
  long double            test_long_double             = 42.42L;

  uint8_t                test_hex_uint8               = 0x42;
  uint16_t               test_hex_uint16              = 0xab42;
  uint32_t               test_hex_uint32              = 0x00abcd42;
  uint64_t               test_hex_uint64              = 0x00abcd4200dcba42;

  // Constant int and floating point types seem to work fine without explicit
  // mentions of the const types in the list in the PT() definition.
  int         const test_const_int         = -42;
  long double const test_const_long_double = 42.42L;

  void              *test_pointer                = NULL;
  void const        *test_pointer_to_const       = NULL;
  // Constant pointers to const appear to match void const * type:
  void const *const  test_const_pointer_to_const = NULL;

#ifdef HAVE_FORMAT_FREE_PRINT_PT_EXTENSIONS_H
  Widget test_widget;
#endif

  printf ("Trying format-free print and trace macros...\n");
  printf ("\n");

  TT (test_char);
  printf ("\n");

  printf ("NOTE: the wchar_t type is not supported because I hate it\n");
  printf ("\n");

  TT (test_string);
  free (test_string);
  test_string = NULL;
  TT (test_const_string);
  TT (test_char_array);
  TT (test_char_const_array);
  TT ("A string literal");  // Fires the char [] handler
  PT ("\n");

  PT ("NOTE: wchar_t strings are not supported because I hate them\n");
  // and wprintf() etc. would likely be required, ug
  PT ("\n");

  TT (test_int);
  TT (test_short_int);
  TT (test_long_int);
  TT (test_long_long_int);
  TT (test_unsigned_short_int);
  TT (test_unsigned_int);
  TT (test_unsigned_long_int);
  TT (test_unsigned_long_long_int);
  PT ("\n");

  PT ("Arguments are guaranteed to be evaluated only once:\n");
  TT (test_int);
  TT (++test_int);
  TT (test_int--);
  TT (test_int);
  PT ("\n");

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
  PT ("\n");

  TT (test_bool);
  TT (false);
  TT (true);
  PT ("\n");

  TTX (test_unsigned_int);
  TTX (test_hex_uint8);
  TTX (test_hex_uint16);
  TTX (test_hex_uint32);
  TTX (test_hex_uint64);
  PT ("\n");

  PT ("Unsigned integer types which aren't size-specific but are "
      "type-compatible with the explicitly supported sized types will "
      "work with the macros that print values in hex:\n");
  TTX (test_unsigned_short_int);
  TTX (test_unsigned_int);
  TTX (test_unsigned_long_int);
  PT ("NOTE however that the unsigned long long int type is apparently NOT "
      "type-compatible with uint64_t, since TTX (test_unsigned_long_long_int) "
      "doesn't work.\n");
  PT ("\n");

  TT (test_float);
  TT (test_double);
  TT (test_long_double);
  PT ("The stringified labels of floating point literals aren't subject "
      "to printf() rounding, but the values themselves are:\n");
  TT (42.424211111F);
  PT ("Weirdly expressed so we can pass with -Wunsuffixed-float-constants:\n");
  TT ((double) 42.424211111L);
  TT (42.424211111L);
  PT ("\n");

  TT (test_const_int);
  TT (test_const_long_double);
  PT ("\n");

  TT (test_pointer);
  TT (test_pointer_to_const);
  TT (test_const_pointer_to_const);
  PT ("\n");

  PT ("Arguments are guaranteed to be evaluated only once:\n");
  TT (i_return_minus_42_as_an_int ());
  TT (test_int);
  TT (++test_int);
  TT (test_int--);
  TT (test_int);
  PT ("\n");

  PT ("Expressions involving function calls work:\n");
  TT (2 * i_return_minus_42_as_an_int ());
  PT ("\n");

  // See the Makefile in the instrument project top-level dir for info on this:
#ifdef HAVE_FORMAT_FREE_PRINT_PT_EXTENSIONS_H
  PT ("Format-free trace of an extended type:\n");
  test_widget.name = strdup ("test_widget");
  // Normally this next call would probably go somewhere else.
  register_printf_specifier ('W', print_widget, print_widget_arginfo);
  TT (&test_widget);
  free (test_widget.name);
  test_widget.name = NULL;
  PT ("\n");
#endif

  PT ("Checkpoint and named checkpoint macros are available:\n");
  CP ();
  NCP (named checkpoint);
  PT ("\n");

  PT ("There are also macros that take an unquoted single-value format "
      "argument:\n");
  TV (test_int, %i);
  PT ("\n");

  PT ("There's a macros to prefix file:line:func: to a format string:\n");
  TS ("at this point test_int is %i", test_int);
  PT ("\n");

  PT ("In addition to the trace macros tested above, there are analogous\n");
  PT ("versions that don't prefix file:line:func, don't add a newline, or\n");
  PT ("die after their ouput.  See the README.md for the full list.\n");
  printf ("\n");

  printf (
      "WARNING: this test program always exits with exit code 0 if it makes "
      "it to this point.  The above output is intended to be manually "
      "inspected for correctness.\n" );
  return 0;
}

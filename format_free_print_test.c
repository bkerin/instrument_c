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
  
  PT ("NOTE: arguments are guaranteed to be evaluated only once:\n"); 
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
  
  // FIXME: what was this next comment supposed to say?  should it be a PT()?
  // Note that unsigned integer types which aren't size-specifice.g.
  TTX (test_hex_uint8);
  TTX (test_hex_uint16);
  TTX (test_hex_uint32);
  TTX (test_hex_uint64);
  PT ("\n");

  TT (test_float);
  TT (test_double);
  TT (test_long_double);
  PT ("NOTE: the stringified labels of floating point literals aren't\n");
  PT ("subject to printf() rounding, but the values themselves are:\n");
  TT (42.424211111F);
  // Weirdly expressed so we can pass with -Wunsuffixed-float-constants: 
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
  
  PT ("NOTE: arguments are guaranteed to be evaluated only once:\n"); 
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
  PT ("Trying format-free trace of an extended type...\n");
  test_widget.name = strdup ("test_widget");
  // Normally this next call would probably go somewhere else.
  register_printf_specifier ('W', print_widget, print_widget_arginfo);
  TT (&test_widget);
  free (test_widget.name);
  test_widget.name = NULL;
  PT ("\n");
#endif

  // Note that the stringified label used for literal floating point values
  // isn't subject to printf() rounding, but the value itself is.
  PT ("Other available variants (mostly tested indirectly by the above):\n");
  PT ("  PT  (thing) -- without source locations or implicit newlines\n");
  PT ("  PL  (thing) -- Like PT(), but adds a Label (e.g. var_name: 42)\n");
  PT ("  DT  (thing) -- Dump Thing.  Like PL(), but also appends a newline\n");
  PT ("  TT  (thing) -- Trace Thing.  Like DT() including file:line:func:\n");
  PT ("  TD  (thing) -- Like TT(), but exit (EXIT_FAILURE) afterwords\n");
  PT ("  ??X (thing) -- Analogous variants to the above, but only work for\n");
  PT ("                uint8_t/uint16_t/uint32_t types and output in hex\n");

  printf ("\n");

  // FIXME: maybe we should actually always fail?  Also note that for some
  // kinds of errors we do fail
  printf (
      "WARNING: this test program always succeeds.  The above output is\n"
      "intended to be *manually* inspected for correctness.\n" );
  return 0;
}

// Extensions to PT() from format_free_print.h
//
// Currently this header just contains a demo that shows how to tell PT()
// how to print new types.

#ifndef INSIDE_FORMAT_FREE_PRINT_H
#  error included from somewhere other than format_free_print.h
#endif

#ifndef FORMAT_FREE_PRINT_PT_EXTENSIONS_H
#define FORMAT_FREE_PRINT_PT_EXTENSIONS_H

// Note that _GNU_SOURCE must be defined when the headers providing the
// extensions we need are included for the *first* time.  This means using the
// -D_GNU_SOURCE gcc option or maybe config.h (if it's always included first).
#ifndef _GNU_SOURCE
#  error GNU libc extensions are required but _GNU_SOURCE is not defined
#endif

#include <stdio.h>
#include <stdlib.h>
#include <printf.h>

// The compiler can't help us type-check printf extensions, since it doesn't
// fully know about them until run-time.  So be extra careful that the format
// code used in handlers in the statement here match the one specified for
// the type in register_printf_specifier().
#define DO_IGNORING_PRINTF_WARNINGS(statement)                         \
  do {                                                                 \
    _Pragma ("GCC diagnostic push");                                   \
    _Pragma ("GCC diagnostic ignored \"-Wformat\"");                   \
    _Pragma ("GCC diagnostic ignored \"-Wformat-extra-args\"");        \
    statement;   /* Be nice and add semi-colon, doesn't hurt here.  */ \
    _Pragma ("GCC diagnostic pop");                                    \
  } while ( 0 )

// This define is where the additional code to be injected into the PT()
// macro goes.
#define FORMAT_FREE_PRINT_PT_ADDITIONAL_WIMCUPSMCS                        \
  /* Here is where your type extensions go.  For example: */              \
  DO_IGNORING_PRINTF_WARNINGS (WIMCUPSMC (Widget *, "%W");)               \
  ;   /* Add final semicolon even though caller does too, for safety.  */

// In real life this stuff would probably be declared and defined elsewhere,
// but to keep this demonstration simple it's here (with static functions
// and the __unused__ attribute to pacify the compiler).

typedef struct
{
  char *name;
} Widget;

__attribute__ ((__unused__))
static int
print_widget (
    FILE *stream,
    struct printf_info const *info,
    void const *const *args)
{
  Widget const *widget;
  char *buffer;
  int len;

  // Format the output into a string
  widget = *((Widget const *const *) (args[0]));
  len = asprintf (
      &buffer, "<Widget %p: %s>", (const void *) widget, widget->name );
  if ( len == -1 ) {
    return -1;
  }

  // Pad to the minimum field width and print to the stream
  len
    = fprintf (
        stream,
        "%*s",
        (info->left ? -info->width : info->width),
        buffer );

  free (buffer);

  return len;
}

__attribute__ ((__unused__))
static int
print_widget_arginfo (struct printf_info const *info, size_t n,
                      int *argtypes, int *size)
{
  info = info;   /* Prevent compiler warning.  */

  // We always take exactly one argument: a pointer to the structure
  if ( n > 0 ) {
    argtypes[0] = PA_POINTER;
    size[0] = sizeof (void *);
  }

  return 1;
}

#endif   // FORMAT_FREE_PRINT_PT_EXTENSIONS_H

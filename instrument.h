// Support for debugging using instrumentation
//
// Certain GCC compilation and linking options should be used:
//
//   * -D_GNU_SOURCE is required at compile time (because GNU libc extensions
//     are required).  Alternately this can be included before the headers
//     providing the extensions are included for the *first* time (but it's
//     a pain to ensure that this is the case).
//
//   * -g is required at compile-time for backtraces and what_func() (and you
//     must not strip binaries or libraries later)
//
//   * -ldl is always required at link-time because dladdr() needs it, if
//     this annoys you and you don't care about looking up functions in
//     shared libs chop out or edit what_func()
//
//   * -O0 can simplify life by preventing the optimizer from inlining
//     functions out of existance.  It's probably not an issue for shared
//     libraries because exported symbols should be safe.  I like to compile
//     twice, once with -O2 and once with -O0, since some useful warnings
//     only fire with one or the other (on gcc at least).
//
//   * For what_func() to work right when looking up pointers to functions in
//     shared librares, -fPIC is required for library *AND* client compilation.
//     FIXMELATER: it's only needed for clients because of dladdr() bugs, if
//     dladdr() gets fixed this requirement can be removed
//
//   * -Wall, -Wextra, -Wformat-signedness, and -Werror aren't required but
//     they make life better.  In my old age I've come to believe in
//     -Wconversion as well (after debugging signed*unsigned bugs created by
//     the best C programmer I've ever seen, https://github.com/ldeniau).
//     The C integer type promotion rules are so counterintuitive with
//     respect to the containment of the sets they model that latent bugs
//     preventable by -Wconversion are guaranteed to happen eventually.

#ifndef INSTRUMENT_H
#define INSTRUMENT_H

// Note that _GNU_SOURCE must be defined when the headers providing the
// extensions we need are included for the *first* time.  This means using the
// -D_GNU_SOURCE gcc option or maybe config.h (if it's always included first).
#ifndef _GNU_SOURCE
#  error GNU libc extensions are required but _GNU_SOURCE is not defined
#endif

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

// File-Line-Function Tuple
#define FLFT __FILE__, __LINE__, __func__

// Check Point: prints source location followed by a newline
#define CP() printf ("%s:%i:%s: checkpoint\n", FLFT)

// Trace Value: given an expression expr and an unquoted format code, print
// the source location and expression text and value followed by a newline,
// e.g. TV (my_int, %i), TV (my_sub_returning_int (), %i).
#define TV(expr, format_code)                                    \
  printf ("%s:%i:%s: " #expr ": " #format_code "\n", FLFT, expr)

// Trace Stuff: print expanded format string in first argument, using values
// given in remaining arguments, tagged with source location and added newline
#ifdef __GNUC__   // This one needs GNU comma-swallowing __VA_ARGS__ extension
#  define TS(fmt, ...) printf ("%s:%i:%s: " fmt "\n", FLFT, ## __VA_ARGS__)
#endif

// If it annoys you that you have to tell printf() that you're printing
// e.g. an int, when the compiler already knows, define this before this
// header is included for the first time and look in the associated header.
// This stuff is in its own optional header to avoid frightening people :)
#ifdef HAVE_INSTRUMENT_FORMAT_FREE_PRINT_H
#  define INSTRUMENT_INSIDE_INSTRUMENT_H
#  include "instrument_format_free_print.h"
#  undef  INSTRUMENT_INSIDE_INSTRUMENT_H
#endif

// Get a new string containing a backtrace for the call point.  This is
// done using the GNU libc backtrace() function and the addr2line program.
// See the notes above about required compiler options.  On error an
// assertion violation is triggered.
//
// Caveats:
//
//   * Stack unwinding involves heuristics.  Callers can do things that defeat
//     it.  If it looks like this routine is lying to you it probably is.
//
//   * This function doesn't make any effort to backtrace through separate
//     (shared or dynamically loaded) libraries.  FIXMELATER: In theory it
//     could, in which case -rdynamic would need to be used when compiling
//     shared libs, and since what_func() would be used per-frame its
//     requirements would need to be met as well.
//
//   * Use from signal handlers probably doesn't work.
//
char *
backtrace_with_line_numbers (void);

// Like assert(), but prints a full backtrace (if NDEBUG isn't defined).
// All caveats of backtrace_with_line_numbers() apply.  */
#ifndef NDEBUG

// Use GCC branch-predicting test for assertions if possible
#  ifdef __GNUC__
#    define INSTRUMENT_MAYBE_EXPECT_FALSE(cond) (__builtin_expect ((cond), 0))
#  else
#    define INSTRUMENT_MAYBE_EXPECT_FALSE(cond) (cond)
#  endif

#  define ASSERT_BT(cond)                                                   \
    do {                                                                    \
      if ( INSTRUMENT_MAYBE_EXPECT_FALSE (!(cond)) ) {                      \
        fprintf (                                                           \
            stderr,                                                         \
            "%s: %s:%i:%s: Assertion ` " #cond "' failed.  Backtrace:\n%s", \
            program_invocation_short_name,                                  \
            FLFT,                                                           \
            backtrace_with_line_numbers() );                                \
        abort ();                                                           \
      }                                                                     \
    } while ( 0 )

#else

#  define ASSERT_BT(COND)

#endif

// Print to stdout a best guess about the function name and source code
// location corresponding to func_addr.  See the notes above about required
// compiler options.  First an attempt is made to look up the address
// in the current binary using the nm program, and if that doesn't find
// anything dladdr() is used to find the correct shared library and nm is
// used on that.  If things appear to have been compiled or stripped such
// that this function cannot succeed an assertion violation is triggered.
// On error an assertion violation is triggered.
void
what_func (void *func_addr);

#endif   // INSTRUMENT_H

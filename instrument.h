// Support for debugging using instrumentation 
//
// Certain GCC compilation and linking options should be used:
//
//   * -g is required at compile-time for backtraces and what_func() (and you
//     must not strip binaries or libraries later)
//
//   * -fpic is required for library *AND* client compilation for what_func()
//     to work when looking up pointers to function in shared libraries
//
//   * -ldl is always required at link-time because dladdr() needs it
//
//   * -rdynamic is required at link-time for backtraces to work
//     FIXME: does it work at all without this?
//
//   * -O0 can simplify life by preventing the optimizer from inlining
//     functions out of existance.  It's probably not an issue for shared
//     libraries because exported symbols should be safe.  I like to compile
//     twice, once with -O2 and once with -O0, since some useful warnings
//     only fire with one or the other.
//
//   * -Wall -Wextra -Werror aren't required but they make life better

#include <stdio.h>
#include <stdlib.h>

// Check Point: prints source location followed by a newline
#define CP() printf ("%s:%i:%s: checkpoint\n", __FILE__, __LINE__, __func__)

// Trace Value: given var name and unquoted print format code, print source
// location and var value followed by a newline, e.g. TV (my_int, %i) 
#define TV(var_name, format_code) TS (#var_name ": " #format_code, var_name)

// Trace Stuff: print expanded format string in first argument, using values
// given in remaining arguments, tagged with source location and added newline
#define TS(fmt, ...) \
  printf ("%s:%i:%s: " fmt "\n", __FILE__, __LINE__, __func__, ## __VA_ARGS__)

// Get a new string containing a backtrace for the call point.  This is
// done using the GNU libc backtrace() function and the addr2line program.
// On error an assertion violation is triggered.
char *
backtrace_with_line_numbers (void);

// Like assert(), but prints a full backtrace (if NDEBUG isn't defined).
// All caveats of backtrace_with_line_numbers() apply.  */
#ifndef NDEBUG
#  define ASSERT_BT(COND)                                                    \
    do {                                                                     \
      if ( __builtin_expect (!(COND), 0) ) {                                 \
        fprintf (                                                            \
            stderr,                                                          \
            "%s: %s:%u: %s: Assertion ` " #COND "' failed.  Backtrace:\n%s", \
            program_invocation_short_name,                                   \
            __FILE__,                                                        \
            __LINE__,                                                        \
            __func__,                                                        \
            backtrace_with_line_numbers() );                                 \
        abort ();                                                            \
      }                                                                      \
    } while ( 0 )
#else
#  define ASSERT_BT(COND)
#endif

// Print to stdout a best guess about the function name and source code
// location corresponding to func_addr.  First an attempt is made to look up
// the address in the current binary using the nm program, and if that doesn't
// find anything dladdr() is used to find the correct shared library and nm
// is used on that.  If things appear to have been compiled or stripped such
// that this function cannot succeed an assertion violation is triggered.
// On error an assertion violation is triggered.
void
what_func (void *func_addr);

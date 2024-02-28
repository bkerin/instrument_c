# What it is

This header-only library is intended to make it easier to debug C code using
instrumentation in the code (rather than a debugger).  It provides stuff for
easy printing of values and also a way to get a backtrace or determine (at
run-time) the name of the function pointed to by a function pointer.

## Print values without needing a format string

Put the `format_free_print.h` in with your other sources and include it:

```C
#include "format_free_print.h"

long unsigned int demo_func(int arg) { return 2*arg; }

int main (void)
{
  int answer = 42;

  DT(answer);              // Dump Thing (with label)
  TT(2.42*answer);         // Also show source location.  Expressions work.
  TD(demo_func(answer));   // Also die after.  TD() arg evaluated only once.

  return 0;
}
```

Will print:

```
answer: 42
print_demo.c:10:main: 2.42*answer: 101.64
print_demo.c:11:main: demo_func(answer): 84
```

There are a few more variants available, here is the full list:

```
  PT  (thing) -- without source locations or implicit newlines
  PL  (thing) -- Like PT(), but adds a Label (e.g. var_name: 42)
  DT  (thing) -- Dump Thing.  Like PL(), but also appends a newline
  TT  (thing) -- Trace Thing.  Like DT() including file:line:func:
  TD  (thing) -- Like TT(), but exit (EXIT_FAILURE) afterwords
  ??X (thing) -- Analogous variants to the above, but only work for
                 uint8_t/uint16_t/uint32_t types and output in hex
```

Most built-in types are handled; see the definition of `PT()` in
`format_free_print.h` for the full list.

There's an extension interface for adding support for additional types; see
`format_free_print_pt_extensions.h` and it's mentions for details.

The output stream to use can be set by defining `FORMAT_FREE_PRINT_STREAM`
(e.g.  to `stderr`).

The number of floating point significant figures can bet set by defining
`FORMAT_FREE_FREE_PRINT_FLOAT_SIGNIFICANT_DIGITS` to a string value like `"6"`.

## Show a backtrace or get the name of a function from a function pointer

WARNING: the `instrument.h` assumes the /proc filesystem is available.

Put `instrument.h` in with your other sources and include it, compile with at
least the `-g` and  option, and link with `-rdynamic` and `-ldl`.  Then:

```C
#include "instrument.h"

void buggy_func(void) { ASSERT_BT (0); }

int main (void)
{
  buggy_func();

  return 0;
}
```

will hopefully show a backtrace.  Note however that stack unwinding involves
heuristics and callers can do things that defeat it.  Also, backtracing through
shared or dynamically loaded libraries isn't fully implemented.

The `-ldl` link option is only required for `what_func_func()`.  If you don't
care about getting the names of functions from function pointers you might
rather just remove `what_func_func()`.  The `-rdynamic` option also might be
unnecessary, so it may be possible to use `instrument.h` without any build
changes except `-g` during compilation.

If you are interested in looking up function names and you're using shared
libraries, you'll need to compile both the library *and* the application with
`-fPIC`.  The `what_func()` macro can then be used to print the name of the
function pointed to by it's argument.

Full backtraces across call stacks including functions from dynamically loaded
libraries are not yet supported (see FIXME items in code).

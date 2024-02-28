# What it is

This header-only library is intended to make it easier to debug C code using
instrumentation in the code (rather than a debugger).  It provides stuff for
easy printing of values and also a way to get a backtrace or determine (at
run-time) the name of the function pointed to by a function pointer.

## Print values without needing a format string

Put the `format_free_print.h` in with your other sources and include it.  Then
if you're using GCC:

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

There are a few more format-free variants available:

```
  PT  (thing) -- without source locations or implicit newlines
  PL  (thing) -- Like PT(), but adds a Label (e.g. var_name: 42)
  DT  (thing) -- Dump Thing.  Like PL(), but also appends a newline
  TT  (thing) -- Trace Thing.  Like DT() but prefix with file:line:func:
  TD  (thing) -- Like TT(), but EXIT_FAILURE afterwords
  ??X (thing) -- Analogous variants to those above, but only work for
                 uint8_t/uint16_t/uint32_t types and output in hex
```

Most built-in types are handled; see the definition of `PT()` in
`format_free_print.h` for the full list.

There's an extension interface for adding support for additional types; see
`format_free_print_pt_extensions.h` and it's mentions for details.

The output stream to use can be set by defining `FORMAT_FREE_PRINT_STREAM`
(e.g.  to `stderr` or some other `FILE` pointer).

The number of floating point significant figures can bet set by defining
`FORMAT_FREE_FREE_PRINT_FLOAT_SIGNIFICANT_DIGITS` to a string value like `"6"`.

Simple checkpoint/die point macros are also availabe:

```
  CP  ()      -- Print e.g. "Hit my_file.c:42:my_func\n"
  NCP (my cp) -- Named CheckPoint, prints e.g. "my cp\n"
  DP  ()      -- Print e.g. "Dying at my_file.c:42:my_func\n" and EXIT_FAILURE
  NDP (my dp) -- Named Die Point, prints e.g. "my dp\n" and EXIT_FAILURE
```

There are also some routines for cases where you do need a format string, (or
for non-gcc compilers where the format-free variants aren't available):

```
  TV  (val, fc) -- Trace Value, e.g. TV (42, %i) will output
                   file:line:func: 42 (no " needed)
  TVD (val, fc) -- Like TV() but die afterwords
  TS  (fs, ...) -- Trace Stuff.  Like fprintf() but honors
                   FORMAT_FREE_PRINT_STREAM and prefixes the output with
                   file-line-func.  This one *does* need " on format the string
  TSD (fs, ...) -- Like TS() but die afterwords
```

### Redefining Death

It's possible to redefine what "death" means, e.g. when working on a loadable
module it may be convenient to "die" with `longjmp()` rather than `exit`.

Code like this in `my_loadable.c`:

```C
#include <setjmp.h>

jmp_buf jmpBuf;

int
some_loadable_module_entry_point (void)
{
  int returnCode = setjmp (jmpBuf);
  if ( returnCode != 0 ) {
    return returnCode;
  }

  module_code ();
}
```

Could be compiled like this:

```
gcc -DFORMAT_FREE_PRINT_DIE='longjmp (jmpBuf, SOME_FAILURE_CODE)' -fPIC -g -O0 mny_loadable.c -o my_loadable.o
```

Of course it's also possible to define `FORMAT_FREE_PRINT_DIE` in the source
itself but care is required to ensure the definition comes before any inclusion
of `format_free_print.h`, or you'll get the default behavior.

## Show a backtrace or get the name of a function from a function pointer

WARNING: the `instrument.h` interface assumes the /proc filesystem is
available.

Put `format_free_print.h` and `instrument.h` in with your other sources,
include `instrument.h`, compile with at least the `-g` and  option, and link
with `-rdynamic` and `-ldl`.  Then:

```C
#include "instrument.h"

void buggy_func(void) { ASSERT_BT (0); }

int main (void)
{
  buggy_func();

  return 0;
}
```

For consistency with `assert()` `ASSERT_BT()` honors `NDEBUG`, always writes to
`stderr`, and always calls `abort()`.  There is also `ASSERT_FFP()` which also
honors `NDEBUG` but writes to `FORMAT_FREE_PRINT_STREAM` and "dies" with
`FORMAT_FREE_PRINT_DIE()`.

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

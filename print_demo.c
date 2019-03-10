// This is the actual source for the print demo in README.md:

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

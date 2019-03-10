// This is the actual source for the backtrace example in README.md:

#include "instrument.h"

void buggy_func(void) { ASSERT_BT (0); }

int main (void)
{
  buggy_func();

  return 0;
}

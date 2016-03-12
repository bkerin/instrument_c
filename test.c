// Implementation of the interface defined in test.h

#include <stdio.h>

#include "test.h"

int
shared_lib_func (void)
{
  printf ("in function %s\n", __func__);

  return 0;
}

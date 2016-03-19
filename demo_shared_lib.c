// Implementation of the interface defined in demo_shared_lib.h

#include <stdio.h>

#include "demo_shared_lib.h"

int
demo_shared_lib_func (void)
{
  printf ("in function %s\n", __func__);

  return 0;
}

#include "pm.h"
#include "mproc.h"

int do_changeparent(void) {
  /* register ? */
  struct mproc *parent = &mproc[mp->mp_parent];
  /* check if process is init */
  if (parent->mp_pid == INIT_PID) {
    return EACCES;
  }
  /* check if process is waiting */
  if ((parent->mp_flags & WAITING)) {
    return EPERM;
  }
  mp->mp_parent = parent->mp_parent;
  return 0;
  
}

#include "pm.h"
#include "mproc.h"

int find(pid_t pid) {
  for (int i = 0; i < NR_PROCS; i++) {
    if (mproc[i].mp_pid == pid) {
      return i;
    }
  }
  return -1;
}

int do_getoppid(void) {
  /* take from message give pid */
  pid_t pid = m_in.m1_i1;
  /* check if given pid exists */
  int idx = find(pid);
  if (idx == -1) {
    return EINVAL;
  }
  pid_t parent = mproc[idx].original_parent;
  mp->mp_reply.m1_i2 = parent;
  return 0;
}

#include <lib.h>
#include <unistd.h>
#include <minix/rs.h>
#include <string.h>

int get_pm_endpt(endpoint_t *pt);

pid_t getoppid(pid_t pid) {
  message m;
  memset(&m, 0, sizeof(m));
  endpoint_t pm_pt;
  if (get_pm_endpt(&pm_pt)) {
    errno = ENOSYS;
    return -1;
  }
  m.m1_i1 = pid;
  if (_syscall(pm_pt, PM_GETOPPID, &m)) {
    return -1;
  }
  return m.m1_i2;
}

#include <lib.h>
#include <unistd.h>
#include <minix/rs.h>
#include <string.h>

int get_pm_endpt(endpoint_t *pt) {
  return minix_rs_lookup("pm", pt);
}

int changeparent(void) {
  message m;
  endpoint_t pm_pt;
  memset(&m, 0, sizeof(m));
  if (get_pm_endpt(&pm_pt)) {
    errno = ENOSYS;
    return -1;
  }
  return _syscall(pm_pt, PM_CHANGE_PARENT, &m);
}

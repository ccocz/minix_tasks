#include <sys/cdefs.h>
#include "namespace.h"
#include <lib.h>

#include <string.h>
#include <unistd.h>

int chroot(name)
const char *name;
{
  message m;

  memset(&m, 0, sizeof(m));
  _loadname(name, &m);
  return(_syscall(VFS_PROC_NR, VFS_CHROOT, &m));
}

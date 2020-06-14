#include <unistd.h>
#include <lib.h>
#include <string.h>
#include <minix/rs.h>

int get_pm_endpt(endpoint_t *pt) {
    return minix_rs_lookup("pm", pt);
}

int givekudos(pid_t pid) {
    message m;
    memset(&m, 0, sizeof(m));
    endpoint_t pm_pt;
    if (get_pm_endpt(&pm_pt)) {
        errno = ENOSYS;
        return -1;
    }
    m.m1_i1 = pid;
    if (_syscall(pm_pt, PM_GIVEKUDOS, &m)) {
        return -1;
    }
    return m.m1_i2;
}
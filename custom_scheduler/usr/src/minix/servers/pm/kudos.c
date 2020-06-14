#include "pm.h"
#include "proto.h"
#include "mproc.h"

int is_parent(struct mproc *child, pid_t parent) {
    while (1) {
        if (child->mp_pid == parent) {
            return 1;
        }
        if (child->mp_pid == INIT_PID) {
            break;
        }
        child = &mproc[child->mp_parent];
    }
    return 0;
}

int get_priority(unsigned kudos) {
    if (kudos < 10) {
        return 3;
    } else if (kudos >= 10 && kudos < 25) {
        return 2;
    } else if (kudos >= 25 && kudos < 50) {
        return 1;
    } else {
        return 0;
    }
}

int do_givekudos(void) {
    /* take argument from message */
    pid_t pid_inc = m_in.m1_i1;
    /* search for the given pid in mproc */
    struct mproc *inc = find_proc(pid_inc);
    /* no such pid */
    if (inc == NULL) {
        return EINVAL;
    }
    /* check scheduler */
    if (inc->mp_scheduler != SCHED_PROC_NR) {
        return EINVAL;
    }
    /* check if caller tries corruption */
    if (is_parent(inc, mp->mp_pid) || is_parent(mp, pid_inc)) {
        return EPERM;
    }
    /* everything fine, change its kudos and priority */
    unsigned new_prio;
    inc->mp_kudos++;
    sched_kudos(inc, inc->mp_kudos);
    mp->mp_reply.m1_i2 = get_priority(inc->mp_kudos);
    return 0;
}
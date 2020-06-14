#include <minix/drivers.h>
#include <minix/chardriver.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <minix/ds.h>
#include <minix/ioctl.h>
#include <sys/ioc_dfa.h>

/* automata properties */

#define NO_STATE 256
#define MAX_LETTER 256

int delta[NO_STATE][MAX_LETTER];
int is_accepting[NO_STATE];
u32_t start_state;
u32_t last_state;

/* Function prototypes for the dfa driver. */
static int dfa_open(devminor_t minor, int access, endpoint_t user_endpt);
static int dfa_close(devminor_t minor);
static ssize_t dfa_read(devminor_t minor, u64_t position, endpoint_t endpt,
                          cp_grant_id_t grant, size_t size, int flags, cdev_id_t id);
static int dfa_ioctl(devminor_t minor, unsigned long request, endpoint_t endpt,
                     cp_grant_id_t grant, int flags, endpoint_t user_endpt, cdev_id_t id);
static ssize_t dfa_write(devminor_t minor, u64_t position, endpoint_t endpt,
                        cp_grant_id_t grant, size_t size, int flags, cdev_id_t id);

/* SEF functions and variables. */
static void sef_local_startup(void);
static int sef_cb_init(int type, sef_init_info_t *info);
static int sef_cb_lu_state_save(int);
static int lu_state_restore(void);

/* Entry points to the dfa driver. */
static struct chardriver dfa_tab =
        {
                .cdr_open	= dfa_open,
                .cdr_close	= dfa_close,
                .cdr_read	= dfa_read,
                .cdr_ioctl  = dfa_ioctl,
                .cdr_write  = dfa_write
        };

static int dfa_open(devminor_t UNUSED(minor), int UNUSED(access),
                      endpoint_t UNUSED(user_endpt))
{
    return OK;
}

static int dfa_close(devminor_t UNUSED(minor))
{
    return OK;
}

static ssize_t dfa_read(devminor_t UNUSED(minor), u64_t UNUSED(position),
                          endpoint_t endpt, cp_grant_id_t grant, size_t size, int UNUSED(flags),
                          cdev_id_t UNUSED(id))
{
    int ret;
    char* buffer = malloc(size);
    if (buffer == NULL) {
        printf("couldn't read fully, malloc failed");
        free(buffer);
        return 0;
    }
    for (int i = 0; i < size; i++) {
        buffer[i] = is_accepting[last_state] ? 'Y' : 'N';
    }
    if ((ret = sys_safecopyto(endpt, grant, 0, (vir_bytes) buffer, size)) != OK) {
        free(buffer);
        return ret;
    }
    free(buffer);
    /* Return the number of bytes read. */
    return size;
}

/* move negative part of char to positive side */
int non_negative(char x) {
    return x >= 0 ? x : 127 - x;
}

static ssize_t dfa_write(devminor_t UNUSED(minor), u64_t UNUSED(position), endpoint_t endpt,
                         cp_grant_id_t grant, size_t size, int UNUSED(flags), cdev_id_t UNUSED(id)) {
    char *buffer = malloc(size);
    if (buffer == NULL) {
        printf("couldn't write fully, malloc failed\n");
        free(buffer);
        return 0;
    }
    int ret = sys_safecopyfrom(endpt, grant, 0, (vir_bytes)buffer, size);
    if (ret != OK) {
        free(buffer);
        return ret;
    }
    for (int i = 0; i < size; i++) {
        last_state = delta[last_state][non_negative(buffer[i])];
    }
    free(buffer);
    return size;
}

static int dfa_ioctl(devminor_t UNUSED(minor), unsigned long request, endpoint_t endpt,
                       cp_grant_id_t grant, int UNUSED(flags), endpoint_t UNUSED(user_endpt), cdev_id_t UNUSED(id)) {
    int rc;
    char value[3];
    switch(request) {
        case DFAIOCRESET:
            last_state = start_state;
            rc = OK;
            break;
        case DFAIOCADD:
            rc = sys_safecopyfrom(endpt, grant, 0, (vir_bytes)value, 3);
            if (rc == OK) {
                delta[non_negative(value[0])][non_negative(value[1])] = non_negative(value[2]);
                last_state = start_state;
            }
            break;
        case DFAIOCACCEPT:
            rc = sys_safecopyfrom(endpt, grant, 0, (vir_bytes)&value[0], 1);
            if (rc == OK) {
                is_accepting[non_negative(value[0])] = 1;
            }
            break;
        case DFAIOCREJECT:
            rc = sys_safecopyfrom(endpt, grant, 0, (vir_bytes)value, 1);
            if (rc == OK) {
                is_accepting[non_negative(value[0])] = 0;
            }
            break;
        default:
            rc = ENOTTY;

    }
    return rc;
}

static int sef_cb_lu_state_save(int UNUSED(state)) {
    /* Save the state. */
    ds_publish_mem("delta", delta, sizeof(delta), DSF_OVERWRITE);
    ds_publish_mem("accepting_states", is_accepting, sizeof(is_accepting), DSF_OVERWRITE);
    ds_publish_u32("start_state", start_state, DSF_OVERWRITE);
    ds_publish_u32("last_state", last_state, DSF_OVERWRITE);
    return OK;
}

static int lu_state_restore() {
    /* Restore the state. */
    u32_t value;

    ds_retrieve_u32("start_state", &value);
    ds_delete_u32("start_state");
    start_state = value;

    ds_retrieve_u32("last_state", &value);
    ds_delete_u32("last_state");
    last_state = value;

    size_t size = sizeof(delta);
    ds_retrieve_mem("delta", (char *) delta, &size);
    ds_delete_mem("delta");

    size = sizeof(is_accepting);
    ds_retrieve_mem("accepting_states", (char *) is_accepting, &size);
    ds_delete_mem("accepting_states");

    return OK;
}

static void sef_local_startup()
{
    /* Register init callbacks. Use the same function for all event types. */
    sef_setcb_init_fresh(sef_cb_init);
    sef_setcb_init_lu(sef_cb_init);
    sef_setcb_init_restart(sef_cb_init);

    /* Register live update callbacks. */
    /* - Agree to update immediately when LU is requested in a valid state. */
    sef_setcb_lu_prepare(sef_cb_lu_prepare_always_ready);
    /* - Support live update starting from any standard state. */
    sef_setcb_lu_state_isvalid(sef_cb_lu_state_isvalid_standard);
    /* - Register a custom routine to save the state. */
    sef_setcb_lu_state_save(sef_cb_lu_state_save);

    /* Let SEF perform startup. */
    sef_startup();
}

static int sef_cb_init(int type, sef_init_info_t *UNUSED(info))
{
    /* Initialize the dfa driver. */
    int do_announce_driver = TRUE;

    switch(type) {
        case SEF_INIT_FRESH:
            break;

        case SEF_INIT_LU:
            /* Restore the state. */
            lu_state_restore();
            do_announce_driver = FALSE;
            break;

        case SEF_INIT_RESTART:
            break;
    }

    /* Announce we are up when necessary. */
    if (do_announce_driver) {
        chardriver_announce();
    }

    /* Initialization completed successfully. */
    return OK;
}

void init_automata() {
    for (int i = 0; i < NO_STATE; i++) {
        for (int j = 0; j < MAX_LETTER; j++) {
            delta[i][j] = 0;
        }
    }
    for (int i = 0; i < NO_STATE; i++) {
        is_accepting[i] = 0;
    }
    start_state = 0;
    last_state = 0;
}

int main(void)
{
    /* initalize automata */
    init_automata();

    /* Perform initialization. */
    sef_local_startup();

    /* Run the main loop. */
    chardriver_task(&dfa_tab);
    return OK;
}

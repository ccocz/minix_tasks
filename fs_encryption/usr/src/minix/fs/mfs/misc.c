#include "fs.h"
#include <assert.h>
#include <minix/vfsif.h>
#include <minix/bdev.h>
#include "inode.h"
#include "clean.h"

/*===========================================================================*
 *				fs_sync					     *
 *===========================================================================*/
int fs_sync()
{
/* Perform the sync() system call.  Flush all the tables. 
 * The order in which the various tables are flushed is critical.  The
 * blocks must be flushed last, since rw_inode() leaves its results in
 * the block cache.
 */
  struct inode *rip;

  assert(lmfs_nr_bufs() > 0);

  /* Write all the dirty inodes to the disk. */
  for(rip = &inode[0]; rip < &inode[NR_INODES]; rip++)
	  if(rip->i_count > 0 && IN_ISDIRTY(rip)) rw_inode(rip, WRITING);

  /* Write all the dirty blocks to the disk. */
  lmfs_flushall();

  return(OK);		/* sync() can't fail */
}


/*===========================================================================*
 *				fs_flush				     *
 *===========================================================================*/
int fs_flush()
{
/* Flush the blocks of a device from the cache after writing any dirty blocks
 * to disk.
 */
  dev_t dev = fs_m_in.m_vfs_fs_flush.device;
  if(dev == fs_dev && lmfs_bufs_in_use() > 0) return(EBUSY);
 
  lmfs_flushall();
  lmfs_invalidate(dev);

  return(OK);
}


/*===========================================================================*
 *				fs_new_driver				     *
 *===========================================================================*/
int fs_new_driver(void)
{
/* Set a new driver endpoint for this device. */
  dev_t dev;
  cp_grant_id_t label_gid;
  size_t label_len;
  char label[sizeof(fs_dev_label)];
  int r;

  dev = fs_m_in.m_vfs_fs_new_driver.device;
  label_gid = fs_m_in.m_vfs_fs_new_driver.grant;
  label_len = fs_m_in.m_vfs_fs_new_driver.path_len;

  if (label_len > sizeof(label))
	return(EINVAL);

  r = sys_safecopyfrom(fs_m_in.m_source, label_gid, (vir_bytes) 0,
	(vir_bytes) label, label_len);

  if (r != OK) {
	printf("MFS: fs_new_driver safecopyfrom failed (%d)\n", r);
	return(EINVAL);
  }

  bdev_driver(dev, label);

  return(OK);
}

int fs_bpeek(void)
{
	return lmfs_do_bpeek(&fs_m_in);
}

int trying_key(ino_t ino) {
    /* function checks if user is trying to
     * manipulate file KEY and sends corresponding responses
     */
    struct inode *root = find_inode(fs_dev, ROOT_INODE);
    if (root == NULL) {
        panic("MFS: couldn't find root inode\n");
        return(EINVAL);
    }
    ino_t numb;
    int e_code = search_dir(root, "KEY", &numb, LOOK_UP, IGN_PERM);
    if (e_code == OK) {
        if (numb == ino) {
            /* user is manipulating to the key */
            return TRYING_KEY;
        } else {
            return KEY_FOUND;
        }
    } else {
        return KEY_NOT_FOUND;
    }
}

int is_not_encrypted(void) {
    /* determines if in the main category exists
     * file not_encrypted */
    struct inode *root = find_inode(fs_dev, ROOT_INODE);
    if (root == NULL) {
        panic("MFS: couldn't find root inode\n");
        return(EINVAL);
    }
    ino_t numb;
    return search_dir(root, "NOT_ENCRYPTED", &numb, LOOK_UP, IGN_PERM);
}
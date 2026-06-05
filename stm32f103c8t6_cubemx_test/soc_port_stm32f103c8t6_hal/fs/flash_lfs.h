#ifndef FLASH_LFS_H
#define FLASH_LFS_H

#include <stdint.h>
#include "lfs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 在内部 Flash 最后 8KB 划分给 LFS (2 个 4KB 扇区) */
#define FLASH_LFS_START  0x0800E000
#define FLASH_LFS_SIZE   0x2000    /* 8 KB */
#define FLASH_LFS_SECT   0x1000    /* 4 KB 扇区 */

/* LFS 配置 */
extern struct lfs_config flash_lfs_cfg;

/* 挂载 LFS (若首次使用或损坏则自动格式化) */
int flash_lfs_mount(lfs_t *lfs);
void flash_lfs_unmount(lfs_t *lfs);

#ifdef __cplusplus
}
#endif

#endif

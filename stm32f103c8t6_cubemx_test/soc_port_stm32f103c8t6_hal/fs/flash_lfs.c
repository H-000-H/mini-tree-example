#include "flash_lfs.h"
#include "main.h"

/* ------------------------------------------------------------------ */
/* 底层 Flash 操作                                                     */
/* ------------------------------------------------------------------ */
static int flash_read(const struct lfs_config *c, lfs_block_t block,
                      lfs_off_t off, void *dst, lfs_size_t size)
{
    (void)c;
    uint32_t addr = FLASH_LFS_START + block * FLASH_LFS_SECT + off;
    memcpy(dst, (const void*)addr, size);
    return 0;
}

static int flash_prog(const struct lfs_config *c, lfs_block_t block,
                      lfs_off_t off, const void *src, lfs_size_t size)
{
    (void)c;
    uint32_t addr = FLASH_LFS_START + block * FLASH_LFS_SECT + off;

    HAL_FLASH_Unlock();
    for (lfs_size_t i = 0; i < size; i += 2)
    {
        uint16_t val;
        memcpy(&val, (const uint8_t*)src + i, 2);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, val) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return LFS_ERR_IO;
        }
    }
    HAL_FLASH_Lock();
    return 0;
}

static int flash_erase(const struct lfs_config *c, lfs_block_t block)
{
    (void)c;
    uint32_t addr = FLASH_LFS_START + block * FLASH_LFS_SECT;

    FLASH_EraseInitTypeDef er = {
        .TypeErase   = FLASH_TYPEERASE_PAGES,
        .PageAddress = addr,
        .NbPages     = FLASH_LFS_SECT / FLASH_PAGE_SIZE,
    };
    uint32_t page_err = 0;

    HAL_FLASH_Unlock();
    HAL_StatusTypeDef st = HAL_FLASHEx_Erase(&er, &page_err);
    HAL_FLASH_Lock();

    return (st == HAL_OK && page_err == 0) ? 0 : LFS_ERR_IO;
}

static int flash_sync(const struct lfs_config *c)
{
    (void)c;
    return 0;
}

/* ------------------------------------------------------------------ */
/* LFS 配置                                                            */
/* ------------------------------------------------------------------ */
struct lfs_config flash_lfs_cfg = {
    .read  = flash_read,
    .prog  = flash_prog,
    .erase = flash_erase,
    .sync  = flash_sync,

    .read_size      = 1,
    .prog_size      = 2,       /* STM32F1 半字编程 */
    .block_size     = FLASH_LFS_SECT,
    .block_count    = FLASH_LFS_SIZE / FLASH_LFS_SECT,
    .cache_size     = 32,
    .lookahead_size = 32,
    .block_cycles   = 500,
};

/* ------------------------------------------------------------------ */
/* 挂载 / 格式化                                                       */
/* ------------------------------------------------------------------ */
int flash_lfs_mount(lfs_t *lfs)
{
    int err = lfs_mount(lfs, &flash_lfs_cfg);
    if (err)
        err = lfs_format(lfs, &flash_lfs_cfg) ? -1 : lfs_mount(lfs, &flash_lfs_cfg);
    return err;
}

void flash_lfs_unmount(lfs_t *lfs)
{
    lfs_unmount(lfs);
}

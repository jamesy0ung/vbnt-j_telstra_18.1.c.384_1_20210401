#ifndef __MTD_NAND_TL_H__
#define __MTD_NAND_TL_H__

#include <linux/mtd/mtd.h>

int mtd_tch_atomic_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf);

#endif /* __MTD_NAND_TL_H__ */

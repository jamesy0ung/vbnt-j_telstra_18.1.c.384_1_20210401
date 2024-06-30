#ifndef __STORAGE_CRC32_H
#define __STORAGE_CRC32_H

int storage_crc32_sb(struct sfs_super_block *sb, unsigned long *crc32);
int storage_crc32_desc(struct sfs_desc *desc, unsigned long *crc32);
int storage_crc32_data(void *data, int size, struct sfs_desc *desc, unsigned long *crc32);

#endif

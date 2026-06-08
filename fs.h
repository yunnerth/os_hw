#ifndef FS_H
#define FS_H

#define MAX_FILES   32      // 最大文件数
#define BLOCK_COUNT 64      // 磁盘总块数
#define BLOCK_SIZE  1024    // 每块大小 (字节)
#define MAX_FILENAME 32
#define MAX_BLOCKS_PER_FILE 16

/* 文件控制块 FCB */
typedef struct {
    int   id;                          // 文件ID
    char  name[MAX_FILENAME];          // 文件名
    int   size;                        // 文件大小 (字节)
    int   blocks[MAX_BLOCKS_PER_FILE]; // 占用的磁盘块号列表
    int   block_count;                 // 占用块数
    int   used;                        // 该FCB是否被占用
} FCB;

/* 简单文件系统 */
typedef struct {
    FCB   files[MAX_FILES];            // 文件目录
    unsigned char bitmap[BLOCK_COUNT]; // 位图: 1=已占用, 0=空闲
    char  disk_blocks[BLOCK_COUNT][BLOCK_SIZE]; // 模拟磁盘块
} SimpleFS;

/* 文件系统操作 */
void fs_init(SimpleFS *fs);
void fs_list(SimpleFS *fs);
void fs_create(SimpleFS *fs, const char *name, int size);
void fs_delete(SimpleFS *fs, const char *name);
void fs_read(SimpleFS *fs, const char *name);
void fs_write(SimpleFS *fs, const char *name, const char *data);
void fs_show_bitmap(SimpleFS *fs);

#endif

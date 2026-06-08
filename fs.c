#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs.h"

/* 初始化文件系统 */
void fs_init(SimpleFS *fs) {
    for (int i = 0; i < MAX_FILES; i++) {
        fs->files[i].id = i;
        fs->files[i].used = 0;
        fs->files[i].name[0] = '\0';
        fs->files[i].size = 0;
        fs->files[i].block_count = 0;
        for (int j = 0; j < MAX_BLOCKS_PER_FILE; j++)
            fs->files[i].blocks[j] = -1;
    }
    for (int i = 0; i < BLOCK_COUNT; i++) {
        fs->bitmap[i] = 0;  // 全部标记为空闲
        memset(fs->disk_blocks[i], 0, BLOCK_SIZE);
    }
    printf("文件系统初始化完成。总块数=%d, 块大小=%dB, 最大文件数=%d\n",
           BLOCK_COUNT, BLOCK_SIZE, MAX_FILES);
}

/* 显示位图 */
void fs_show_bitmap(SimpleFS *fs) {
    int free_blocks = 0;
    printf("\n=== 磁盘位图 (0=空闲 1=占用) ===\n");
    for (int i = 0; i < BLOCK_COUNT; i++) {
        if (i % 16 == 0) printf("\n块%2d: ", i);
        printf("%d", fs->bitmap[i]);
        if (fs->bitmap[i] == 0) free_blocks++;
    }
    printf("\n\n空闲块数: %d / %d\n", free_blocks, BLOCK_COUNT);
}

/* 找到一个空闲FCB */
static int find_free_fcb(SimpleFS *fs) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!fs->files[i].used) return i;
    }
    return -1;
}

/* 按文件名查找文件，返回FCB下标 */
static int find_file(SimpleFS *fs, const char *name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->files[i].used && strcmp(fs->files[i].name, name) == 0)
            return i;
    }
    return -1;
}

/* 分配N个连续空闲块（首次适应），返回分配的起始块号 */
static int allocate_blocks(SimpleFS *fs, int count, int *out_blocks) {
    int allocated = 0;
    for (int i = 0; i < BLOCK_COUNT && allocated < count; i++) {
        if (fs->bitmap[i] == 0) {
            out_blocks[allocated] = i;
            fs->bitmap[i] = 1;
            allocated++;
        }
    }
    if (allocated < count) {
        // 回滚
        for (int j = 0; j < allocated; j++)
            fs->bitmap[out_blocks[j]] = 0;
        return 0;
    }
    return 1;
}

/* 列出所有文件 */
void fs_list(SimpleFS *fs) {
    printf("\n=== 文件目录 ===\n");
    int found = 0;
    printf("%-6s %-20s %-10s %-8s %s\n", "ID", "文件名", "大小(B)", "块数", "占用块");
    printf("------------------------------------------------------------\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->files[i].used) {
            found = 1;
            printf("%-6d %-20s %-10d %-8d ",
                   fs->files[i].id,
                   fs->files[i].name,
                   fs->files[i].size,
                   fs->files[i].block_count);
            for (int j = 0; j < fs->files[i].block_count; j++)
                printf("%d ", fs->files[i].blocks[j]);
            printf("\n");
        }
    }
    if (!found) printf("(空)\n");
}

/* 创建文件 */
void fs_create(SimpleFS *fs, const char *name, int size) {
    if (strlen(name) >= MAX_FILENAME) {
        printf("错误: 文件名过长 (最大%d字符)\n", MAX_FILENAME - 1);
        return;
    }
    if (find_file(fs, name) != -1) {
        printf("错误: 文件 '%s' 已存在\n", name);
        return;
    }
    int fcb_idx = find_free_fcb(fs);
    if (fcb_idx == -1) {
        printf("错误: 文件目录已满\n");
        return;
    }

    // 计算所需块数
    int blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (blocks_needed == 0) blocks_needed = 1;
    if (blocks_needed > MAX_BLOCKS_PER_FILE) {
        printf("错误: 文件太大 (最大%d块)\n", MAX_BLOCKS_PER_FILE);
        return;
    }

    int block_list[MAX_BLOCKS_PER_FILE];
    if (!allocate_blocks(fs, blocks_needed, block_list)) {
        printf("错误: 磁盘空间不足\n");
        return;
    }

    FCB *f = &fs->files[fcb_idx];
    strcpy(f->name, name);
    f->size = size;
    f->block_count = blocks_needed;
    f->used = 1;
    for (int i = 0; i < blocks_needed; i++)
        f->blocks[i] = block_list[i];
    for (int i = blocks_needed; i < MAX_BLOCKS_PER_FILE; i++)
        f->blocks[i] = -1;

    printf("创建文件成功: %s (ID=%d, 大小=%dB, 占用%d块)\n",
           name, f->id, size, blocks_needed);
}

/* 删除文件 */
void fs_delete(SimpleFS *fs, const char *name) {
    int idx = find_file(fs, name);
    if (idx == -1) {
        printf("错误: 文件 '%s' 不存在\n", name);
        return;
    }

    FCB *f = &fs->files[idx];
    // 释放占用的块
    for (int i = 0; i < f->block_count; i++) {
        fs->bitmap[f->blocks[i]] = 0;
        memset(fs->disk_blocks[f->blocks[i]], 0, BLOCK_SIZE);
    }
    printf("删除文件: %s (释放%d块)\n", name, f->block_count);

    // 清空FCB
    f->name[0] = '\0';
    f->size = 0;
    f->block_count = 0;
    f->used = 0;
    for (int i = 0; i < MAX_BLOCKS_PER_FILE; i++)
        f->blocks[i] = -1;
}

/* 读文件 */
void fs_read(SimpleFS *fs, const char *name) {
    int idx = find_file(fs, name);
    if (idx == -1) {
        printf("错误: 文件 '%s' 不存在\n", name);
        return;
    }

    FCB *f = &fs->files[idx];
    printf("\n读取文件 '%s' (大小=%dB, 块数=%d):\n", f->name, f->size, f->block_count);
    for (int i = 0; i < f->block_count; i++) {
        int blk = f->blocks[i];
        printf("  块%d: ", blk);
        // 打印块内容（最多显示前64字节）
        int show = f->size - i * BLOCK_SIZE;
        if (show > 64) show = 64;
        if (show > BLOCK_SIZE) show = BLOCK_SIZE;
        for (int j = 0; j < show; j++) {
            char c = fs->disk_blocks[blk][j];
            if (c == '\0') printf(".");
            else printf("%c", c);
        }
        printf("\n");
    }
}

/* 写文件 */
void fs_write(SimpleFS *fs, const char *name, const char *data) {
    int idx = find_file(fs, name);
    if (idx == -1) {
        printf("错误: 文件 '%s' 不存在\n", name);
        return;
    }

    FCB *f = &fs->files[idx];
    int data_len = (int)strlen(data);

    // 检查是否超过文件容量
    int capacity = f->block_count * BLOCK_SIZE;
    if (data_len > capacity) {
        printf("警告: 数据被截断 (写入%d字节, 文件容量%d字节)\n", data_len, capacity);
        data_len = capacity;
    }

    int offset = 0;
    for (int i = 0; i < f->block_count && offset < data_len; i++) {
        int blk = f->blocks[i];
        int write_size = data_len - offset;
        if (write_size > BLOCK_SIZE) write_size = BLOCK_SIZE;
        memcpy(fs->disk_blocks[blk], data + offset, write_size);
        offset += write_size;
    }

    printf("写入文件 '%s' 成功 (%d字节)\n", name, data_len);
}

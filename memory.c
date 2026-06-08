#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "memory.h"

/* ========== 动态分区分配 ========== */

Partition* init_memory(int total_size) {
    Partition *head = (Partition *)malloc(sizeof(Partition));
    head->id = -1;  // -1 表示空闲分区
    head->start_addr = 0;
    head->size = total_size;
    head->next = NULL;
    return head;
}

/* 释放所有分区链 */
void reset_memory(Partition *head, int total_size) {
    Partition *p = head->next;
    while (p) {
        Partition *t = p;
        p = p->next;
        free(t);
    }
    head->id = -1;
    head->start_addr = 0;
    head->size = total_size;
    head->next = NULL;
}

/* 打印当前分区状态 */
void print_partitions(Partition *head) {
    printf("\n--- 当前内存分区状态 ---\n");
    printf("%-10s %-12s %-10s\n", "分区编号", "起始地址", "大小");
    Partition *p = head;
    while (p) {
        if (p->id == -1)
            printf("%-10s %-12d %-10d\n", "空闲", p->start_addr, p->size);
        else
            printf("进程P%-7d %-12d %-10d\n", p->id, p->start_addr, p->size);
        p = p->next;
    }
}

/* 首次适应算法 (First Fit) */
void first_fit(Partition *head, int pid, int size) {
    Partition *p = head;
    int found = 0;

    while (p) {
        if (p->id == -1 && p->size >= size) {
            int remaining = p->size - size;

            if (remaining > 0) {
                // 分割分区
                Partition *new_free = (Partition *)malloc(sizeof(Partition));
                new_free->id = -1;
                new_free->start_addr = p->start_addr + size;
                new_free->size = remaining;
                new_free->next = p->next;

                p->id = pid;
                p->size = size;
                p->next = new_free;
            } else {
                // 正好完全分配
                p->id = pid;
            }
            found = 1;
            printf("FF: 进程P%d 分配 %d KB, 起始地址 %d\n", pid, size, p->start_addr);
            break;
        }
        p = p->next;
    }

    if (!found)
        printf("FF: 进程P%d 分配 %d KB 失败——无足够连续空间\n", pid, size);
}

/* 最佳适应算法 (Best Fit) */
void best_fit(Partition *head, int pid, int size) {
    Partition *p = head;
    Partition *best = NULL;

    // 找到满足需求的最小空闲分区
    while (p) {
        if (p->id == -1 && p->size >= size) {
            if (best == NULL || p->size < best->size)
                best = p;
        }
        p = p->next;
    }

    if (best == NULL) {
        printf("BF: 进程P%d 分配 %d KB 失败——无足够连续空间\n", pid, size);
        return;
    }

    int remaining = best->size - size;
    if (remaining > 0) {
        Partition *new_free = (Partition *)malloc(sizeof(Partition));
        new_free->id = -1;
        new_free->start_addr = best->start_addr + size;
        new_free->size = remaining;
        new_free->next = best->next;

        best->id = pid;
        best->size = size;
        best->next = new_free;
    } else {
        best->id = pid;
    }
    printf("BF: 进程P%d 分配 %d KB, 起始地址 %d\n", pid, size, best->start_addr);
}

/* 释放进程占用的分区，并与相邻空闲分区合并 */
void free_partition(Partition *head, int pid) {
    Partition *p = head;
    int released = 0;

    while (p) {
        if (p->id == pid) {
            p->id = -1;
            released = 1;
            printf("释放进程P%d 占用的分区 (起始=%d, 大小=%d)\n",
                   pid, p->start_addr, p->size);

            // 与下一个分区合并（如果它也空闲）
            if (p->next && p->next->id == -1) {
                Partition *t = p->next;
                p->size += t->size;
                p->next = t->next;
                free(t);
            }
            break;
        }
        p = p->next;
    }

    if (!released) {
        printf("未找到进程P%d 的分区\n", pid);
        return;
    }

    // 检查前一个分区是否空闲，合并
    Partition *prev = head;
    while (prev && prev->next != p) prev = prev->next;
    if (prev && prev->id == -1) {
        prev->size += p->size;
        prev->next = p->next;
        free(p);
    }
}

/* ========== 页面置换算法 ========== */

/* 检查页是否已在帧中，返回下标，未找到返回-1 */
static int find_page(PageFrame frames[], int frame_count, int page_id) {
    for (int i = 0; i < frame_count; i++) {
        if (frames[i].page_id == page_id)
            return i;
    }
    return -1;
}

/* 找到最久装入的帧（FIFO 淘汰） */
static int find_fifo_victim(PageFrame frames[], int frame_count) {
    int min_time = INT_MAX, victim = 0;
    for (int i = 0; i < frame_count; i++) {
        if (frames[i].load_time < min_time) {
            min_time = frames[i].load_time;
            victim = i;
        }
    }
    return victim;
}

/* 找到最久未用的帧（LRU 淘汰） */
static int find_lru_victim(PageFrame frames[], int frame_count) {
    int min_use = INT_MAX, victim = 0;
    for (int i = 0; i < frame_count; i++) {
        if (frames[i].last_use < min_use) {
            min_use = frames[i].last_use;
            victim = i;
        }
    }
    return victim;
}

/* FIFO 页面置换 */
void fifo_page_replace(int ref_string[], int len, int frame_count) {
    printf("\n===== FIFO 页面置换 (帧数=%d) =====\n", frame_count);
    printf("引用串: ");
    for (int i = 0; i < len; i++) printf("%d ", ref_string[i]);
    printf("\n\n");

    PageFrame *frames = (PageFrame *)malloc(frame_count * sizeof(PageFrame));
    for (int i = 0; i < frame_count; i++) {
        frames[i].page_id = -1;
        frames[i].load_time = -1;
    }

    int page_faults = 0, time = 0;
    for (int i = 0; i < len; i++) {
        int page = ref_string[i];
        int pos = find_page(frames, frame_count, page);

        if (pos != -1) {
            printf("时刻%d: 访问页%d → 命中 [", time, page);
        } else {
            page_faults++;
            // 找空闲帧或淘汰
            int slot = -1;
            for (int j = 0; j < frame_count; j++) {
                if (frames[j].page_id == -1) { slot = j; break; }
            }
            if (slot == -1) {
                slot = find_fifo_victim(frames, frame_count);
            }
            frames[slot].page_id = page;
            frames[slot].load_time = time;
            printf("时刻%d: 访问页%d → 缺页(装入帧%d) [", time, page, slot);
        }

        for (int j = 0; j < frame_count; j++) {
            if (frames[j].page_id == -1) printf("空 ");
            else printf("%d ", frames[j].page_id);
        }
        printf("]\n");
        time++;
    }

    printf("\n缺页次数: %d, 缺页率: %.2f%%\n",
           page_faults, 100.0 * page_faults / len);
    free(frames);
}

/* LRU 页面置换 */
void lru_page_replace(int ref_string[], int len, int frame_count) {
    printf("\n===== LRU 页面置换 (帧数=%d) =====\n", frame_count);
    printf("引用串: ");
    for (int i = 0; i < len; i++) printf("%d ", ref_string[i]);
    printf("\n\n");

    PageFrame *frames = (PageFrame *)malloc(frame_count * sizeof(PageFrame));
    for (int i = 0; i < frame_count; i++) {
        frames[i].page_id = -1;
        frames[i].last_use = -1;
    }

    int page_faults = 0, time = 0;
    for (int i = 0; i < len; i++) {
        int page = ref_string[i];
        int pos = find_page(frames, frame_count, page);

        if (pos != -1) {
            frames[pos].last_use = time;
            printf("时刻%d: 访问页%d → 命中 [", time, page);
        } else {
            page_faults++;
            int slot = -1;
            for (int j = 0; j < frame_count; j++) {
                if (frames[j].page_id == -1) { slot = j; break; }
            }
            if (slot == -1) {
                slot = find_lru_victim(frames, frame_count);
            }
            frames[slot].page_id = page;
            frames[slot].last_use = time;
            printf("时刻%d: 访问页%d → 缺页(装入帧%d) [", time, page, slot);
        }

        for (int j = 0; j < frame_count; j++) {
            if (frames[j].page_id == -1) printf("空 ");
            else printf("%d ", frames[j].page_id);
        }
        printf("]\n");
        time++;
    }

    printf("\n缺页次数: %d, 缺页率: %.2f%%\n",
           page_faults, 100.0 * page_faults / len);
    free(frames);
}

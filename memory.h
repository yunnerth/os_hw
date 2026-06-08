#ifndef MEMORY_H
#define MEMORY_H

/* ---- 动态分区分配 ---- */
typedef struct Partition {
    int id;                 // 分区编号，-1表示空闲
    int start_addr;         // 起始地址
    int size;               // 分区大小
    struct Partition *next;
} Partition;

Partition* init_memory(int total_size);
void first_fit(Partition *head, int pid, int size);
void best_fit(Partition *head, int pid, int size);
void free_partition(Partition *head, int pid);
void print_partitions(Partition *head);
void reset_memory(Partition *head, int total_size);

/* ---- 页面置换 ---- */
typedef struct {
    int page_id;
    int load_time;   // 装入时刻（FIFO用）
    int last_use;    // 最近使用时刻（LRU用）
} PageFrame;

void fifo_page_replace(int ref_string[], int len, int frame_count);
void lru_page_replace(int ref_string[], int len, int frame_count);

#endif

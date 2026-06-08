#include <stdio.h>
#include <stdlib.h>
#include "scheduler.h"
#include "memory.h"
#include "sync.h"
#include "fs.h"

/* ---- 丢弃当前行剩余内容 ---- */
static void flush_line() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* ---- 健壮的整数输入：每次读完后自动清行尾 ---- */
static int read_int() {
    int val;
    while (scanf("%d", &val) != 1) {
        printf("输入无效，请重新输入整数: ");
        flush_line();
    }
    return val;
}

/* ========== 1. 处理机调度子菜单 ========== */
void menu_scheduler() {
    int choice, n;

    while (1) {
        printf("\n========== 处理机调度 ==========\n");
        printf("1. FCFS 先来先服务\n");
        printf("2. SJF  短作业优先\n");
        printf("3. RR   时间片轮转\n");
        printf("4. 优先级调度\n");
        printf("0. 返回主菜单\n");
        printf("请选择: ");
        choice = read_int();
        flush_line();
        if (choice == 0) break;

        printf("请输入进程数量: ");
        n = read_int();
        flush_line();
        if (n <= 0) {
            printf("进程数必须大于0!\n");
            continue;
        }
        Process *proc = (Process *)malloc(n * sizeof(Process));

        if (choice == 4)
            printf("请输入每个进程的 (到达时间 服务时间 优先级):\n");
        else
            printf("请输入每个进程的 (到达时间 服务时间):\n");

        for (int i = 0; i < n; i++) {
            proc[i].id = i + 1;
            printf("P%d: ", i + 1);

            proc[i].arrival_time = read_int();
            proc[i].burst_time   = read_int();
            if (choice == 4)
                proc[i].priority = read_int();
            else
                proc[i].priority = 0;

            proc[i].remaining_time = proc[i].burst_time;
            proc[i].waiting_time = 0;
            proc[i].turnaround_time = 0;
            proc[i].started = 0;

            flush_line();  /* 丢弃该行多余输入，防止泄漏到下一进程 */
        }

        switch (choice) {
            case 1: fcfs(proc, n); break;
            case 2: sjf(proc, n); break;
            case 3: {
                int q;
                printf("请输入时间片大小: ");
                q = read_int();
                flush_line();
                rr(proc, n, q);
                break;
            }
            case 4: priority_schedule(proc, n); break;
        }
        free(proc);
    }
}

/* ========== 2. 内存管理子菜单 ========== */
void menu_memory() {
    int choice;
    Partition *mem = NULL;
    int total_size = 0, init_done = 0;

    while (1) {
        printf("\n========== 内存管理 ==========\n");
        printf("1. 初始化内存\n");
        printf("2. 首次适应 (FF) 分配\n");
        printf("3. 最佳适应 (BF) 分配\n");
        printf("4. 释放分区\n");
        printf("5. 显示分区状态\n");
        printf("6. 页面置换 - FIFO\n");
        printf("7. 页面置换 - LRU\n");
        printf("0. 返回主菜单\n");
        printf("请选择: ");
        choice = read_int();
        flush_line();

        if (choice == 0) {
            if (mem) { free(mem); mem = NULL; }
            break;
        }

        if (choice == 1) {
            if (mem) { free(mem); mem = NULL; }
            printf("请输入内存总大小 (KB): ");
            total_size = read_int();
            flush_line();
            mem = init_memory(total_size);
            init_done = 1;
            print_partitions(mem);
        } else if (choice >= 2 && choice <= 5 && !init_done) {
            printf("请先初始化内存!\n");
            continue;
        } else if (choice == 2 || choice == 3) {
            int pid, size;
            printf("请输入进程ID和所需大小(KB): ");
            pid  = read_int();
            size = read_int();
            flush_line();
            if (choice == 2)
                first_fit(mem, pid, size);
            else
                best_fit(mem, pid, size);
            print_partitions(mem);
        } else if (choice == 4) {
            int pid;
            printf("请输入要释放的进程ID: ");
            pid = read_int();
            flush_line();
            free_partition(mem, pid);
            print_partitions(mem);
        } else if (choice == 5) {
            print_partitions(mem);
        } else if (choice == 6 || choice == 7) {
            int len, frame_count;
            printf("请输入引用串长度: ");
            len = read_int();
            flush_line();
            if (len <= 0) continue;
            int *ref = (int *)malloc(len * sizeof(int));
            printf("请输入引用串 (空格分隔): ");
            for (int i = 0; i < len; i++)
                ref[i] = read_int();
            flush_line();
            printf("请输入物理帧数: ");
            frame_count = read_int();
            flush_line();

            if (choice == 6)
                fifo_page_replace(ref, len, frame_count);
            else
                lru_page_replace(ref, len, frame_count);
            free(ref);
        }
    }
}

/* ========== 3. 进程同步子菜单 ========== */
void menu_sync() {
    int choice;
    while (1) {
        printf("\n========== 进程同步 ==========\n");
        printf("1. 生产者-消费者问题\n");
        printf("2. 读者-写者问题\n");
        printf("3. 哲学家进餐问题\n");
        printf("0. 返回主菜单\n");
        printf("请选择: ");
        choice = read_int();
        flush_line();

        if (choice == 0) break;
        switch (choice) {
            case 1: producer_consumer_demo(); break;
            case 2: reader_writer_demo(); break;
            case 3: dining_philosophers_demo(); break;
        }
    }
}

/* ========== 4. 文件系统子菜单 ========== */
void menu_fs() {
    SimpleFS fs;
    fs_init(&fs);
    int choice;

    while (1) {
        printf("\n========== 文件系统 ==========\n");
        printf("1. 创建文件\n");
        printf("2. 删除文件\n");
        printf("3. 读写文件\n");
        printf("4. 查看文件\n");
        printf("5. 显示所有文件\n");
        printf("6. 显示位图\n");
        printf("0. 返回主菜单\n");
        printf("请选择: ");
        choice = read_int();
        flush_line();

        if (choice == 0) break;

        char name[MAX_FILENAME];
        char data[512];
        int size;

        switch (choice) {
            case 1:
                printf("文件名 大小(B): ");
                scanf("%s %d", name, &size);
                flush_line();
                fs_create(&fs, name, size);
                break;
            case 2:
                printf("文件名: ");
                scanf("%s", name);
                flush_line();
                fs_delete(&fs, name);
                break;
            case 3:
                printf("文件名 数据内容: ");
                scanf("%s %s", name, data);
                flush_line();
                fs_write(&fs, name, data);
                printf("--- 读取验证 ---\n");
                fs_read(&fs, name);
                break;
            case 4:
                printf("文件名: ");
                scanf("%s", name);
                flush_line();
                fs_read(&fs, name);
                break;
            case 5:
                fs_list(&fs);
                break;
            case 6:
                fs_show_bitmap(&fs);
                break;
        }
    }
}

/* ========== 主菜单 ========== */
int main() {
    int choice;

    while (1) {
        printf("\n");
        printf("╔══════════════════════════════════╗\n");
        printf("║    操作系统课程设计 - 基础模块   ║\n");
        printf("╠══════════════════════════════════╣\n");
        printf("║  1. 处理机调度                  ║\n");
        printf("║     (FCFS/SJF/RR/优先级)        ║\n");
        printf("║  2. 内存管理                    ║\n");
        printf("║     (FF/BF, FIFO/LRU)           ║\n");
        printf("║  3. 进程同步                    ║\n");
        printf("║     (生产者/读者/哲学家)        ║\n");
        printf("║  4. 文件系统                    ║\n");
        printf("║     (文件管理/位图空闲管理)     ║\n");
        printf("║  0. 退出                        ║\n");
        printf("╚══════════════════════════════════╝\n");
        printf("请选择: ");
        choice = read_int();
        flush_line();

        switch (choice) {
            case 1: menu_scheduler(); break;
            case 2: menu_memory(); break;
            case 3: menu_sync(); break;
            case 4: menu_fs(); break;
            case 0:
                printf("再见!\n");
                return 0;
            default:
                printf("无效选择，请重试。\n");
        }
    }
    return 0;
}

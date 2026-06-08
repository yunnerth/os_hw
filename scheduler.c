#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "scheduler.h"

/* ---------- 辅助函数 ---------- */

static void sort_by_arrival(Process proc[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (proc[j].arrival_time > proc[j + 1].arrival_time) {
                Process tmp = proc[j];
                proc[j] = proc[j + 1];
                proc[j + 1] = tmp;
            }
        }
    }
}

static void calc_and_print(Process proc[], int n) {
    double avg_wt = 0, avg_tat = 0;
    printf("\n进程\t到达\t服务\t等待\t周转\n");
    for (int i = 0; i < n; i++) {
        printf("P%d\t%d\t%d\t%d\t%d\n",
               proc[i].id,
               proc[i].arrival_time,
               proc[i].burst_time,
               proc[i].waiting_time,
               proc[i].turnaround_time);
        avg_wt += proc[i].waiting_time;
        avg_tat += proc[i].turnaround_time;
    }
    printf("\n平均等待时间: %.2f\n", avg_wt / n);
    printf("平均周转时间: %.2f\n", avg_tat / n);
}

/* ---------- 1. 先来先服务 FCFS ---------- */
void fcfs(Process proc[], int n) {
    printf("\n===== FCFS 先来先服务调度 =====\n");
    sort_by_arrival(proc, n);

    int current_time = 0;
    for (int i = 0; i < n; i++) {
        if (current_time < proc[i].arrival_time)
            current_time = proc[i].arrival_time;

        proc[i].waiting_time = current_time - proc[i].arrival_time;
        current_time += proc[i].burst_time;
        proc[i].finish_time = current_time;
        proc[i].turnaround_time = proc[i].finish_time - proc[i].arrival_time;
    }

    calc_and_print(proc, n);
    // 打印甘特图
    printf("甘特图: ");
    current_time = 0;
    sort_by_arrival(proc, n);
    for (int i = 0; i < n; i++) {
        if (current_time < proc[i].arrival_time)
            current_time = proc[i].arrival_time;
        printf("[P%d] ", proc[i].id);
        current_time += proc[i].burst_time;
    }
    printf("\n");
}

/* ---------- 2. 短作业优先 SJF（非抢占） ---------- */
void sjf(Process proc[], int n) {
    printf("\n===== SJF 短作业优先调度（非抢占） =====\n");

    int *done = (int *)calloc(n, sizeof(int));
    int completed = 0, current_time = 0;

    while (completed < n) {
        int idx = -1;
        int min_burst = INT_MAX;

        // 在已到达的进程中选最短作业
        for (int i = 0; i < n; i++) {
            if (!done[i] && proc[i].arrival_time <= current_time
                && proc[i].burst_time < min_burst) {
                min_burst = proc[i].burst_time;
                idx = i;
            }
        }

        if (idx == -1) {
            current_time++;  // 没有进程到达，推进时间
        } else {
            proc[idx].waiting_time = current_time - proc[idx].arrival_time;
            current_time += proc[idx].burst_time;
            proc[idx].finish_time = current_time;
            proc[idx].turnaround_time = proc[idx].finish_time - proc[idx].arrival_time;
            done[idx] = 1;
            completed++;
            printf("执行 P%d (服务时间=%d)\n", proc[idx].id, proc[idx].burst_time);
        }
    }

    free(done);
    calc_and_print(proc, n);
}

/* ---------- 3. 时间片轮转 RR ---------- */
void rr(Process proc[], int n, int quantum) {
    printf("\n===== RR 时间片轮转调度 (时间片=%d) =====\n", quantum);

    /* 按到达时间排序 */
    sort_by_arrival(proc, n);

    for (int i = 0; i < n; i++)
        proc[i].remaining_time = proc[i].burst_time;

    int *queue = (int *)malloc(n * n * sizeof(int)); /* 就绪队列，足够大 */
    int front = 0, rear = 0;
    int completed = 0, current_time = 0;
    int *done = (int *)calloc(n, sizeof(int));
    int *started = (int *)calloc(n, sizeof(int));
    int next_arrival = 0;  /* 下一个将要到达的进程下标 */

    /* 初始时刻已到达的进程入队 */
    while (next_arrival < n && proc[next_arrival].arrival_time <= current_time)
        queue[rear++] = next_arrival++;

    /* CPU空闲且队列空但有进程尚未到达时，快进时间 */
    if (front == rear && completed < n) {
        current_time = proc[next_arrival].arrival_time;
        while (next_arrival < n && proc[next_arrival].arrival_time <= current_time)
            queue[rear++] = next_arrival++;
    }

    printf("甘特图: ");

    while (completed < n) {
        /* 队列空但还有进程未到达 */
        if (front == rear) {
            current_time = proc[next_arrival].arrival_time;
            while (next_arrival < n && proc[next_arrival].arrival_time <= current_time)
                queue[rear++] = next_arrival++;
            continue;
        }

        int idx = queue[front++];
        started[idx] = 1;

        int run_time = (proc[idx].remaining_time <= quantum)
                       ? proc[idx].remaining_time : quantum;

        printf("[P%d] ", proc[idx].id);
        current_time += run_time;
        proc[idx].remaining_time -= run_time;

        /* 将运行期间新到达的进程入队 */
        while (next_arrival < n && proc[next_arrival].arrival_time <= current_time) {
            queue[rear++] = next_arrival++;
        }

        if (proc[idx].remaining_time == 0) {
            proc[idx].finish_time = current_time;
            proc[idx].turnaround_time = proc[idx].finish_time - proc[idx].arrival_time;
            proc[idx].waiting_time = proc[idx].turnaround_time - proc[idx].burst_time;
            done[idx] = 1;
            completed++;
        } else {
            /* 未完成，重新入队等待下一轮 */
            queue[rear++] = idx;
        }
    }
    printf("\n");

    free(queue);
    free(done);
    free(started);
    calc_and_print(proc, n);
}

/* ---------- 4. 优先级调度（非抢占，数值越小优先级越高） ---------- */
void priority_schedule(Process proc[], int n) {
    printf("\n===== 优先级调度（非抢占，数值越小优先级越高） =====\n");

    int *done = (int *)calloc(n, sizeof(int));
    int completed = 0, current_time = 0;

    while (completed < n) {
        int idx = -1;
        int highest_priority = INT_MAX;

        // 在已到达的进程中选优先级最高的
        for (int i = 0; i < n; i++) {
            if (!done[i] && proc[i].arrival_time <= current_time
                && proc[i].priority < highest_priority) {
                highest_priority = proc[i].priority;
                idx = i;
            }
        }

        if (idx == -1) {
            current_time++;
        } else {
            proc[idx].waiting_time = current_time - proc[idx].arrival_time;
            current_time += proc[idx].burst_time;
            proc[idx].finish_time = current_time;
            proc[idx].turnaround_time = proc[idx].finish_time - proc[idx].arrival_time;
            done[idx] = 1;
            completed++;
            printf("执行 P%d (优先级=%d, 服务时间=%d)\n",
                   proc[idx].id, proc[idx].priority, proc[idx].burst_time);
        }
    }

    free(done);
    calc_and_print(proc, n);
}

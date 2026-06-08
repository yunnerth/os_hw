#ifndef SCHEDULER_H
#define SCHEDULER_H

typedef struct {
    int id;             // 进程ID
    int arrival_time;   // 到达时间
    int burst_time;     // 服务时间（CPU区间）
    int remaining_time; // 剩余执行时间
    int priority;       // 优先级（数值越小优先级越高）
    int waiting_time;   // 等待时间
    int turnaround_time;// 周转时间
    int finish_time;    // 完成时间
    int started;        // 是否已开始执行
} Process;

void fcfs(Process proc[], int n);
void sjf(Process proc[], int n);
void rr(Process proc[], int n, int quantum);
void priority_schedule(Process proc[], int n);

#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "sync.h"

/* ================================================================
   1. 生产者-消费者问题 (有界缓冲区)
      - empty: 空槽位数信号量（初值 = BUFFER_SIZE）
      - full:  已占用槽位数信号量（初值 = 0）
      - mutex: 互斥访问缓冲区
   ================================================================ */

#define BUFFER_SIZE  5
#define PRODUCE_COUNT 8   // 每个生产者生产的产品数
#define CONSUME_COUNT 8   // 每个消费者消费的产品数 (总数需匹配: 2×8 产 = 2×8 消)

static int buffer[BUFFER_SIZE];
static int in = 0, out = 0;

static sem_t empty, full;
static pthread_mutex_t pc_mutex;

static int produced_total = 0;   // 已生产总数
static int consumed_total = 0;   // 已消费总数

static void *producer(void *arg) {
    int id = *(int *)arg;

    for (int i = 0; i < PRODUCE_COUNT; i++) {
        int item = id * 1000 + i;  // 产品编号

        // 模拟生产耗时
        usleep((rand() % 500 + 100) * 1000);  // 100-600ms

        sem_wait(&empty);              // P(empty) — 等空位
        pthread_mutex_lock(&pc_mutex);

        buffer[in] = item;
        printf("[生产者P%d] 生产产品#%d → 放入 buffer[%d] (当前缓冲:",
               id, item, in);
        in = (in + 1) % BUFFER_SIZE;
        for (int j = 0, idx = out; j < (in - out + BUFFER_SIZE) % BUFFER_SIZE; j++) {
            printf(" %d", buffer[idx]);
            idx = (idx + 1) % BUFFER_SIZE;
        }
        printf(" )\n");

        produced_total++;
        pthread_mutex_unlock(&pc_mutex);
        sem_post(&full);               // V(full) — 通知有新产品
    }

    printf("[生产者P%d] 生产结束\n", id);
    return NULL;
}

static void *consumer(void *arg) {
    int id = *(int *)arg;

    for (int i = 0; i < CONSUME_COUNT; i++) {
        sem_wait(&full);               // P(full) — 等产品
        pthread_mutex_lock(&pc_mutex);

        int item = buffer[out];
        printf("[消费者C%d] 消费产品#%d ← 取自 buffer[%d] (当前缓冲:",
               id, item, out);
        out = (out + 1) % BUFFER_SIZE;
        for (int j = 0, idx = out; j < (in - out + BUFFER_SIZE) % BUFFER_SIZE; j++) {
            printf(" %d", buffer[idx]);
            idx = (idx + 1) % BUFFER_SIZE;
        }
        printf(" )\n");

        consumed_total++;
        pthread_mutex_unlock(&pc_mutex);
        sem_post(&empty);              // V(empty) — 通知有空位

        // 模拟消费耗时
        usleep((rand() % 800 + 200) * 1000);  // 200-1000ms
    }

    printf("[消费者C%d] 消费结束\n", id);
    return NULL;
}

void producer_consumer_demo() {
    printf("\n===== 生产者-消费者问题 (pthread + semaphore) =====\n");
    printf("缓冲区大小: %d | 2个生产者(各产%d个) | 2个消费者(各消费%d个)\n\n",
           BUFFER_SIZE, PRODUCE_COUNT, CONSUME_COUNT);

    sem_init(&empty, 0, BUFFER_SIZE);
    sem_init(&full,  0, 0);
    pthread_mutex_init(&pc_mutex, NULL);
    produced_total = consumed_total = 0;

    pthread_t producers[2], consumers[2];
    int p_ids[2] = {1, 2};
    int c_ids[2] = {1, 2};

    for (int i = 0; i < 2; i++) {
        pthread_create(&producers[i], NULL, producer, &p_ids[i]);
        pthread_create(&consumers[i], NULL, consumer, &c_ids[i]);
    }

    for (int i = 0; i < 2; i++) {
        pthread_join(producers[i], NULL);
        pthread_join(consumers[i], NULL);
    }

    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&pc_mutex);

    printf("\n生产者-消费者演示完成。共生产=%d, 共消费=%d\n\n",
           produced_total, consumed_total);
}

/* ================================================================
   2. 读者-写者问题 (读者优先)
      - rw_mutex: 读写互斥量，写者或第一个读者占用
      - mutex:    保护 read_count 的互斥量
      - read_count: 当前正在读的读者数量
   ================================================================ */

static pthread_mutex_t rw_mutex;  // 读写互斥
static pthread_mutex_t rc_mutex;  // 保护 read_count
static int read_count = 0;
static int shared_data = 100;     // 共享数据
static int rw_running = 1;        // 控制线程运行

static void *reader(void *arg) {
    int id = *(int *)arg;
    int reads = 0;

    while (rw_running && reads < 5) {
        usleep((rand() % 400 + 100) * 1000);  // 思考时间

        pthread_mutex_lock(&rc_mutex);
        read_count++;
        if (read_count == 1)
            pthread_mutex_lock(&rw_mutex);    // 第一个读者锁住写者
        pthread_mutex_unlock(&rc_mutex);

        // ===== 读操作 =====
        printf("[读者R%d] 开始读... read_count=%d, data=%d\n",
               id, read_count, shared_data);
        usleep((rand() % 300 + 100) * 1000);  // 读取耗时
        reads++;

        pthread_mutex_lock(&rc_mutex);
        printf("[读者R%d] 读完, read_count=%d→%d\n",
               id, read_count, read_count - 1);
        read_count--;
        if (read_count == 0)
            pthread_mutex_unlock(&rw_mutex);  // 最后一个读者释放写者
        pthread_mutex_unlock(&rc_mutex);
    }
    return NULL;
}

static void *writer(void *arg) {
    int id = *(int *)arg;
    int writes = 0;

    while (rw_running && writes < 3) {
        usleep((rand() % 600 + 200) * 1000);  // 思考时间

        pthread_mutex_lock(&rw_mutex);         // P(rw_mutex)

        // ===== 写操作 =====
        shared_data += 10;
        printf("[写者W%d] ★ 正在写入... data=%d\n", id, shared_data);
        usleep((rand() % 500 + 200) * 1000);  // 写入耗时
        writes++;

        pthread_mutex_unlock(&rw_mutex);       // V(rw_mutex)
    }
    return NULL;
}

void reader_writer_demo() {
    printf("\n===== 读者-写者问题 (读者优先, pthread + mutex) =====\n");
    printf("共享数据初值=%d | 3个读者(各读5次) | 2个写者(各写3次)\n\n",
           shared_data);

    pthread_mutex_init(&rw_mutex, NULL);
    pthread_mutex_init(&rc_mutex, NULL);
    read_count = 0;
    shared_data = 100;
    rw_running = 1;

    pthread_t readers[3], writers[2];
    int r_ids[3] = {1, 2, 3};
    int w_ids[2] = {1, 2};

    for (int i = 0; i < 3; i++)
        pthread_create(&readers[i], NULL, reader, &r_ids[i]);
    for (int i = 0; i < 2; i++)
        pthread_create(&writers[i], NULL, writer, &w_ids[i]);

    // 等待写者完成
    for (int i = 0; i < 2; i++)
        pthread_join(writers[i], NULL);
    // 等待读者完成
    for (int i = 0; i < 3; i++)
        pthread_join(readers[i], NULL);

    pthread_mutex_destroy(&rw_mutex);
    pthread_mutex_destroy(&rc_mutex);

    printf("\n读者-写者演示完成。最终 data=%d\n\n", shared_data);
}

/* ================================================================
   3. 哲学家进餐问题
      5位哲学家, 5根筷子(semaphore), 限制最多N-1人同时尝试进餐
      - chopsticks[i]: 每根筷子的信号量（初值=1）
      - limit: 限制同时进餐人数的信号量（初值=N-1），防止死锁
      - mutex: 保护屏幕输出
   ================================================================ */

#define N_PHILO   5
#define EAT_TIMES 3   // 每位哲学家进餐次数

static sem_t chopsticks[N_PHILO];
static sem_t limit;        // 限制最多 N-1 个人同时拿筷子
static pthread_mutex_t print_mutex;  // 保护打印输出

static void *philosopher(void *arg) {
    int id = *(int *)arg;
    int left  = id;                    // 左手边的筷子
    int right = (id + 1) % N_PHILO;   // 右手边的筷子

    for (int i = 0; i < EAT_TIMES; i++) {
        // 思考
        pthread_mutex_lock(&print_mutex);
        printf("[哲学家%d] 思考中... (第%d次)\n", id, i + 1);
        pthread_mutex_unlock(&print_mutex);
        usleep((rand() % 600 + 200) * 1000);  // 200-800ms

        // 饥饿，尝试拿筷子
        pthread_mutex_lock(&print_mutex);
        printf("[哲学家%d] 饥饿，等待筷子 %d 和 %d\n", id, left, right);
        pthread_mutex_unlock(&print_mutex);

        sem_wait(&limit);             // 先获取"限流"信号量（防死锁）
        sem_wait(&chopsticks[left]);  // 拿左手筷子
        sem_wait(&chopsticks[right]); // 拿右手筷子

        // 进餐
        pthread_mutex_lock(&print_mutex);
        printf("[哲学家%d] ▲ 拿起筷子%d和%d，正在进餐... (第%d次)\n",
               id, left, right, i + 1);
        pthread_mutex_unlock(&print_mutex);
        usleep((rand() % 500 + 100) * 1000);  // 100-600ms

        // 放下筷子
        sem_post(&chopsticks[right]);
        sem_post(&chopsticks[left]);
        sem_post(&limit);

        pthread_mutex_lock(&print_mutex);
        printf("[哲学家%d] 放下筷子%d和%d，继续思考\n", id, left, right);
        pthread_mutex_unlock(&print_mutex);
    }

    pthread_mutex_lock(&print_mutex);
    printf("[哲学家%d] ☆ 已完成%d次进餐，离开餐桌\n", id, EAT_TIMES);
    pthread_mutex_unlock(&print_mutex);
    return NULL;
}

void dining_philosophers_demo() {
    printf("\n===== 哲学家进餐问题 (pthread + semaphore) =====\n");
    printf("5位哲学家, 5根筷子, 限制最多%d人同时尝试拿筷子(防死锁)\n", N_PHILO - 1);
    printf("每位哲学家进餐 %d 次\n\n", EAT_TIMES);

    // 初始化信号量
    for (int i = 0; i < N_PHILO; i++)
        sem_init(&chopsticks[i], 0, 1);
    sem_init(&limit, 0, N_PHILO - 1);  // 关键：最多4人同时拿筷子
    pthread_mutex_init(&print_mutex, NULL);

    pthread_t threads[N_PHILO];
    int ids[N_PHILO];
    for (int i = 0; i < N_PHILO; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, philosopher, &ids[i]);
    }

    for (int i = 0; i < N_PHILO; i++)
        pthread_join(threads[i], NULL);

    // 销毁信号量
    for (int i = 0; i < N_PHILO; i++)
        sem_destroy(&chopsticks[i]);
    sem_destroy(&limit);
    pthread_mutex_destroy(&print_mutex);

    printf("\n哲学家进餐演示完成。\n\n");
}

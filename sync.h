#ifndef SYNC_H
#define SYNC_H

/* 生产者-消费者问题 (有界缓冲区，使用信号量) */
void producer_consumer_demo();

/* 读者-写者问题 (读者优先，使用互斥锁) */
void reader_writer_demo();

/* 哲学家进餐问题 (使用信号量避免死锁) */
void dining_philosophers_demo();

#endif

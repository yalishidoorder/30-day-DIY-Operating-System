// FIFO缓冲区相关

#include "my_bootpack.h"

#define FLAGS_OVERRUN		0x0001

// 初始化FIFO缓冲区
void fifo32_init(struct FIFO32* fifo, int size, int* buf, struct TASK* task)
{
	fifo->size = size;
	fifo->buf = buf;
	fifo->free = size; /* 缓冲区置空 */
	fifo->flags = 0;
	fifo->n_w = 0; /* 下一个数据写入位置 */
	fifo->n_r = 0; /* 下一个数据读出位置 */
    fifo->task = task; /* 有数据写入时需要唤醒的任务 */ 

	return;
}

// 向FIFO传送数据并保存，同时唤醒任务 
int fifo32_put(struct FIFO32* fifo, int data)
{
    if (fifo->free == 0) {
        /* 空余不足，溢出 */
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }

    fifo->buf[fifo->n_w] = data;
    fifo->n_w++;

    if (fifo->n_w == fifo->size) {
        fifo->n_w = 0;
    }
    fifo->free--;

    if (fifo->task != 0) {
        if (fifo->task->flags != TASK_RUNNING) {  /* 如果任务处于休眠状态 */
            task_run(fifo->task, -1, 0); /* 将任务唤醒，不改变优先级和level */
        }
    }

    return 0;
}

// 从FIFO取得一个数据 
int fifo32_get(struct FIFO32* fifo)
{
    int data;
    if (fifo->free == fifo->size) {
        /* 如果缓冲区为空，则返回 -1 */
        return -1;
    }

    data = fifo->buf[fifo->n_r];
    fifo->n_r++;

    if (fifo->n_r == fifo->size) {
        fifo->n_r = 0;
    }
    fifo->free++;

    return data;
}

// 报告一下到底积攒了多少数据 
int fifo32_status(struct FIFO32* fifo)
{
    return fifo->size - fifo->free;
}

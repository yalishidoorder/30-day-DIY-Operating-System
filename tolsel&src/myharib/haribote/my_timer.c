// 计时器相关

#include <stdio.h>
#include "my_bootpack.h"

#define PIT_CTRL	0x0043
#define PIT_CNT0	0x0040

#define TIMER_FLAGS_ALLOC       1   /* 已配置状态 */
#define TIMER_FLAGS_USING       2   /* 定时器运行中 */

struct TIMERCTL timerctl;

// 初始化可编程间隔定时器
void init_pit(void)
{
	int i;
	io_out8(PIT_CTRL, 0x34);
	// 将中断周期设置成11932，中断频率为100hz
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);

	timerctl.count = 0;
	for (i = 0; i < MAX_TIMER; i++) {
		timerctl.timers0[i].flags = 0; /* 未使用 */
	}

	timerctl.t_head = timer_alloc();
	timerctl.t_head->timeout = 0xffffffff;
	timerctl.t_head->flags = TIMER_FLAGS_USING;
	timerctl.t_head->next_timer = 0;

	timerctl.next_time = 0xffffffff; /* 因为最初没有正在运行的定时器 */

	return;
}

// 分配
struct TIMER* timer_alloc(void)
{
	int i;
	for (i = 0; i < MAX_TIMER; i++) {
		if (timerctl.timers0[i].flags == 0) {
			timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
			timerctl.timers0[i].flags2 = NO_SHUTDOWN;
			return &timerctl.timers0[i];
		}
	}
	return 0; /* 没找到 */
}

// 释放
void timer_free(struct TIMER* timer)
{
	timer->flags = 0; /* 未使用 */
	return;
}

void timer_init(struct TIMER* timer, struct FIFO32* fifo, int data)
{
	timer->fifo = fifo;
	timer->data = data;

	return;
}

// 删去重复插入的timer
void timer_remove(struct TIMER* timer)
{
	struct TIMER* curr = timerctl.t_head;
	struct TIMER* prev = 0;
	while (curr->next_timer != 0) {
		if (curr == timer) {
			if (prev == 0) {
				timerctl.t_head = curr->next_timer;
			}
			else {
				prev->next_timer = curr->next_timer;
			}
			break;
		}
		prev = curr;
		curr = curr->next_timer;
	}

	return;
}

void timer_insert(struct TIMER* timer)
{
	struct TIMER* current, * prev = 0;

	current = timerctl.t_head;

	/* 搜寻插入位置 */
	while (1) {
		if (timer->timeout <= current->timeout) {
			if (prev == 0) {
				timerctl.t_head = timer;
				timerctl.next_time = timer->timeout;
			}
			else {
				prev->next_timer = timer;
			}
			timer->next_timer = current;

			return;
		}

		prev = current;
		current = current->next_timer;
	}
}

// 定时
void timer_settime(struct TIMER* timer, unsigned int timeout)
{
	// 当前的timer重复插入
	if (timer->flags == TIMER_FLAGS_USING) {
		// ✅ 如果当前 timer 已经在链表中，要把它移除
		timer_remove(timer);
	}

	int e;
	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_USING;

	// 关闭中断
	e = io_load_eflags();
	io_cli();

	timer_insert(timer);

	io_store_eflags(e);

	return;
}

// IRQ0发生时调用的中断处理程序
void inthandler20(int* esp)
{
	int ts = 0;
	struct TIMER* timer;

	io_out8(PIC0_OCW2, 0x60);   /* 把IRQ-00信号接收完了的信息通知给PIC */
	timerctl.count++;

	if (timerctl.count < timerctl.next_time) {
		return; /* 还不到下一个时刻，所以结束*/
	}

	timer = timerctl.t_head;

	int e = io_load_eflags();
	io_cli();
	while (1) {
		/* 因为timers的定时器都处于运行状态，所以不确认flags */
		if (timer->timeout > timerctl.count) {
			break;
		}
		/* 超时 */
		timer->flags = TIMER_FLAGS_ALLOC;
		if (timer != task_timer) {
			fifo32_put(timer->fifo, timer->data);
			//ts = 0;
		}
		else {
			ts = 1; /* task_timer超时了 */
		}
		
		timer = timer->next_timer; /* 将下一个定时器的地址赋给timer */
	}
	timerctl.t_head = timer;
	timerctl.next_time = timer->timeout;

	if (ts != 0) {
		task_switch();
	}

	io_store_eflags(e);

	return;
}

int timer_cancel(struct TIMER* timer)
{
	int e;
	struct TIMER* t;
	e = io_load_eflags();
	io_cli();   /* 在设置过程中禁止改变定时器状态 */

	if (timer->flags == TIMER_FLAGS_USING) {    /* 是否需要取消？*/
		/* 第一个定时器的取消处理 */
		if (timer == timerctl.t_head) {
			t = timer->next_timer;
			timerctl.t_head = t;
			timerctl.next_time = t->timeout;
		}
		/* 非第一个定时器的取消处理 */
		else {
			/* 找到timer前一个定时器 */
			t = timerctl.t_head;
			while (t->next_timer != timer) {
				t = t->next_timer;
			}
			t->next_timer = timer->next_timer; 
			/* 将之前“timer的下一个”指向“timer的下一个”*/
		}
		timer->flags = TIMER_FLAGS_ALLOC;

		io_store_eflags(e);
		return 1;   /* 取消处理成功 */
	}

	io_store_eflags(e);
	return 0; /* 不需要取消处理 */
}

void timer_cancelall(struct FIFO32* fifo)
{
	int e, i;
	struct TIMER* t;
	e = io_load_eflags();
	io_cli();   /*在设置过程中禁止改变定时器状态*/

	for (i = 0; i < MAX_TIMER; i++) {
		t = &timerctl.timers0[i];
		if (t->flags != 0 && t->flags2 != NO_SHUTDOWN && t->fifo == fifo) {
			timer_cancel(t);
			timer_free(t);
		}
	}

	io_store_eflags(e);
	return;
}

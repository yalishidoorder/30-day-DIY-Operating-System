/* 插入关系 */

#include <stdio.h>
#include "my_bootpack.h"

// PIC初始化 
void init_pic(void)
{
	io_out8(PIC0_IMR, 0xff); /* 不接受所有的中断 */
	io_out8(PIC1_IMR, 0xff); /* 不接受所有的中断 */

	io_out8(PIC0_ICW1, 0x11); /* 边缘触发模式 */
	io_out8(PIC0_ICW2, 0x20); /* IRQ0-7在INT20-27中接收 */
	io_out8(PIC0_ICW3, 1 << 2); /* PIC1连接到IRQ2 */
	io_out8(PIC0_ICW4, 0x01); /* 非缓冲模式 */

	io_out8(PIC1_ICW1, 0x11); /* 边缘触发模式 */
	io_out8(PIC1_ICW2, 0x28); /* IRQ8-15偼丄INT28-2f偱庴偗傞 */
	io_out8(PIC1_ICW3, 2); /* PIC1连接到IRQ2 */
	io_out8(PIC1_ICW4, 0x01); /* 非缓冲模式 */

	io_out8(PIC0_IMR, 0xfb); /* 11111011 除了PIC1，其他全部禁止 */
	io_out8(PIC1_IMR, 0xff); /* 11111111 不接受所有的中断 */

	return;
}

void inthandler27(int* esp)
/* 来自PIC0的非完全中断对策 */
/* 在Athlon64X2等设备中，由于芯片组的原因，在PIC初始化时这个中断只会发生一次 */
/* 这个中断处理函数对此中断不做任何处理，直接忽略掉 */
/* 为什么可以不做任何处理？
→ 这个中断是由于PIC初始化时的电气噪声引起的，
   所以没有必要认真去处理它。 */
{
	io_out8(PIC0_OCW2, 0x67); /* IRQ-07已完成接收并通知PIC */
	return;
}

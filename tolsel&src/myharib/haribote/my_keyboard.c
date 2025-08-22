// 键盘相关

#include "my_bootpack.h"

struct FIFO32 *keyfifo;
int keydata0;

// PS/2键盘的中断
void inthandler21(int* esp)
{
	int data;
	/* 通知PIC0"IRQ-01已经受理完毕",将“0x60+IRQ号码”输出给OCW2*/
	io_out8(PIC0_OCW2, 0x61);
	data = io_in8(PORT_KEYDAT);

	fifo32_put(keyfifo, data + keydata0);

	return;
}

#define PORT_KEYSTA				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

// 等待键盘控制电路准备完毕 
void wait_KBC_sendready(void)
{
	while (1) {
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}

	return;
}

// 初始化键盘控制电路 
void init_keyboard(struct FIFO32* fifo, int data0)
{
	// 将FIFO缓冲区的信息保存到全局变量里
	keyfifo = fifo;
	keydata0 = data0;

	// 键盘控制器初始化
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);

	return;
}

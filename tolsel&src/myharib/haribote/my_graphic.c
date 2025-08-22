/* 图形处理相关 */
#include "my_bootpack.h"

// 初始化调色板
void init_palette(void)
{
	static unsigned char table_rgb[16 * 3] = {
		0x00, 0x00, 0x00,	/*  0:黑 */
		0xff, 0x00, 0x00,   /*  1:亮红 */
		0x00, 0xff, 0x00,   /*  2:亮绿 */
		0xff, 0xff, 0x00,   /*  3:亮黄 */
		0x00, 0x00, 0xff,   /*  4:亮蓝 */
		0xff, 0x00, 0xff,   /*  5:亮紫 */
		0x00, 0xff, 0xff,   /*  6:浅亮蓝 */
		0xff, 0xff, 0xff,   /*  7:白 */
		0xc6, 0xc6, 0xc6,   /*  8:亮灰 */
		0x84, 0x00, 0x00,   /*  9:暗红 */
		0x00, 0x84, 0x00,   /* 10:暗绿 */
		0x84, 0x84, 0x00,   /* 11:暗黄 */
		0x00, 0x00, 0x84,   /* 12:暗青 */
		0x84, 0x00, 0x84,   /* 13:暗紫 */
		0x00, 0x84, 0x84,   /* 14:浅暗蓝 */
		0x84, 0x84, 0x84    /* 15:暗灰 */
	};
	unsigned char table2[216 * 3];
	int r, g, b;
	set_palette(0, 15, table_rgb);
	// 216种颜色
	for (b = 0; b < 6; b++) {
		for (g = 0; g < 6; g++) {
			for (r = 0; r < 6; r++) {
				table2[(r + g * 6 + b * 36) * 3 + 0] = r * 51;
				table2[(r + g * 6 + b * 36) * 3 + 1] = g * 51;
				table2[(r + g * 6 + b * 36) * 3 + 2] = b * 51;
			}
		}
	}
	set_palette(16, 231, table2);
	return;

	/* C语言中的static char语句只能用于数据，相当于汇编中的DB指令 */
}

// 设定调色板
void set_palette(int start, int end, unsigned char* rgb)
{
	int i, eflags;
	eflags = io_load_eflags();  /* 记录中断许可标志的值*/
	io_cli();                   /* 将中断许可标志置为0，禁止中断 */
	io_out8(0x03c8, start);
	for (i = start; i <= end; i++) {
		io_out8(0x03c9, rgb[0] / 4);
		io_out8(0x03c9, rgb[1] / 4);
		io_out8(0x03c9, rgb[2] / 4);
		rgb += 3;
	}
	io_store_eflags(eflags);    /* 复原中断许可标志 */
	return;
}

// 绘制矩形函数
// 像素坐标（x,y）对应的VRAM地址应按该式计算：0xa0000 + x + y * 320
void boxfill8(unsigned char* vram, int xsize,
	unsigned char c, int x0, int y0, int x1, int y1)
	/* 参数含义
	* unsigned char* vram ： 图像数据随机存储器
	* int xsize ： 地址计算方式中的参数320，即屏幕分辨率的x值
	* unsigned char c ： 像素点颜色、
	* int x0, int y0 ： 起始x,y坐标
	* int x1, int y1 ： 终点x,y坐标
	*/
{
	int x, y;
	for (y = y0; y <= y1; y++) {
		for (x = x0; x <= x1; x++)
			vram[y * xsize + x] = c;
	}
	return;
}

// 初始化屏幕显示函数
void init_screen8(char* vram, int x, int y)
{
	boxfill8(vram, x, COL8_0000FF, 0, 0, x - 1, y - 29);
	boxfill8(vram, x, COL8_C6C6C6, 0, y - 28, x - 1, y - 28);
	boxfill8(vram, x, COL8_FFFFFF, 0, y - 27, x - 1, y - 27);
	boxfill8(vram, x, COL8_C6C6C6, 0, y - 26, x - 1, y - 1);

	boxfill8(vram, x, COL8_FFFFFF, 3, y - 24, 59, y - 24);
	boxfill8(vram, x, COL8_FFFFFF, 2, y - 24, 2, y - 4);
	boxfill8(vram, x, COL8_848484, 3, y - 4, 59, y - 4);
	boxfill8(vram, x, COL8_848484, 59, y - 23, 59, y - 5);
	boxfill8(vram, x, COL8_000000, 2, y - 3, 59, y - 3);
	boxfill8(vram, x, COL8_000000, 60, y - 24, 60, y - 3);

	boxfill8(vram, x, COL8_848484, x - 47, y - 24, x - 4, y - 24);
	boxfill8(vram, x, COL8_848484, x - 47, y - 23, x - 47, y - 4);
	boxfill8(vram, x, COL8_FFFFFF, x - 47, y - 3, x - 4, y - 3);
	boxfill8(vram, x, COL8_FFFFFF, x - 3, y - 24, x - 3, y - 3);
	return;
}

// 输出单个字符/图形函数
void putfont8(char* vram, int xsize, int x,
	int y, char c, char* font)
	/* 参数含义
	* char* vram ： 图像数据随机存储器
	* int xsize ： 地址计算方式中的参数320，即屏幕分辨率的x值
	* int x, int y ： 起始点x,y坐标（左上角）
	* char c ： 像素点颜色
	* char *font ： 16进制表示的 8x16 点阵数组，用于表示字符/图形
	*/
{
	int i;
	char* p, d;

	for (i = 0; i < length; i++) {
		p = vram + (y + i) * xsize + x;
		d = font[i];
		if ((d & 0x80) != 0) { p[0] = c; }
		if ((d & 0x40) != 0) { p[1] = c; }
		if ((d & 0x20) != 0) { p[2] = c; }
		if ((d & 0x10) != 0) { p[3] = c; }
		if ((d & 0x08) != 0) { p[4] = c; }
		if ((d & 0x04) != 0) { p[5] = c; }
		if ((d & 0x02) != 0) { p[6] = c; }
		if ((d & 0x01) != 0) { p[7] = c; }
	}

	return;
}

// 输出字符串（字符数组）函数
void putfonts8_asc(char* vram, int xsize, int x,
	int y, char c, unsigned char* s)
	/* 参数含义
	* char* vram ： 图像数据随机存储器
	* int xsize ： 地址计算方式中的参数320，即屏幕分辨率的x值
	* int x, int y ： 起始点x,y坐标（左上角）
	* char c ： 像素点颜色
	* unsigned char* s ： 字符数组起始指针
	*/
{
	// hankaku的字体数据与字符的对应关系：char *(hankaku) = hankaku + ascii * 16
	extern char hankaku[4096];
	struct TASK* task = task_now();
	char* nihongo = (char*)*((int*)0x0fe8), * font;
	int k, t;  // 区号和点号

	if (task->langmode == ENGLISH) {
		for (; *s != 0x00; s++) {                               
			putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
			x += 8;
		}                                                       
	}
	else if (task->langmode == NIHONGO) {
		for (; *s != 0x00; s++) {
			if (task->langbyte1 == 0) {
				if ((0x81 <= *s && *s <= 0x9f) || (0xe0 <= *s && *s <= 0xfc)) {
					task->langbyte1 = *s;
				}
				else {
					putfont8(vram, xsize, x, y, c, nihongo + *s * 16);
				}
			}
			else {
				if (0x81 <= task->langbyte1 && task->langbyte1 <= 0x9f) {
					k = (task->langbyte1 - 0x81) * 2;
				}
				else {
					k = (task->langbyte1 - 0xe0) * 2 + 62;
				}
				if (0x40 <= *s && *s <= 0x7e) {
					t = *s - 0x40;
				}
				else if (0x80 <= *s && *s <= 0x9e) {
					t = *s - 0x80 + 63;
				}
				else {
					t = *s - 0x9f;
					k++;
				}
				task->langbyte1 = 0;
				font = nihongo + 256 * 16 + (k * 94 + t) * 32;
				putfont8(vram, xsize, x - 8, y, c, font);	/* 左半部分 */
				putfont8(vram, xsize, x, y, c, font + 16);	/* 右半部分 */
			}
			x += 8;
		}
	}
	else if (task->langmode == EUC) {
		for (; *s != 0x00; s++) {
			if (task->langbyte1 == 0) {
				if (0x81 <= *s && *s <= 0xfe) {
					task->langbyte1 = *s;
				}
				else {
					putfont8(vram, xsize, x, y, c, nihongo + *s * 16);
				}
			}
			else {
				// euc下区号点号的计算方法
				k = task->langbyte1 - 0xa1;
				t = *s - 0xa1;
				task->langbyte1 = 0;
				font = nihongo + 256 * 16 + (k * 94 + t) * 32;
				putfont8(vram, xsize, x - 8, y, c, font);  /*左半部分*/
				putfont8(vram, xsize, x, y, c, font + 16);  /*右半部分*/
			}
			x += 8;
		}
	}

	return;
}

// 准备鼠标指针（16×16） ,bc为背景颜色 
void init_mouse_cursor8(char* mouse, char bc)
{
	static char cursor[16][16] = {
		"**************..",
		"*OOOOOOOOOOO*...",
		"*OOOOOOOOOO*....",
		"*OOOOOOOOO*.....",
		"*OOOOOOOO*......",
		"*OOOOOOO*.......",
		"*OOOOOOO*.......",
		"*OOOOOOOO*......",
		"*OOOO**OOO*.....",
		"*OOO*..*OOO*....",
		"*OO*....*OOO*...",
		"*O*......*OOO*..",
		"**........*OOO*.",
		"*..........*OOO*",
		"............*OO*",
		".............***"
	};
	int x, y;
	for (y = 0; y < 16; y++) {
		for (x = 0; x < 16; x++) {
			if (cursor[y][x] == '*') {
				mouse[y * 16 + x] = COL8_000000;
			}
			if (cursor[y][x] == 'O') {
				mouse[y * 16 + x] = COL8_FFFFFF;
			}
			if (cursor[y][x] == '.') {
				mouse[y * 16 + x] = bc;
			}
		}
	}
	return;
}

// 绘制鼠标指针函数
void putblock8_8(char* vram, int vxsize, int pxsize,
	int pysize, int px0, int py0, char* buf, int bxsize)
	/* 参数含义
	* char *vram ： 图像数据随机存储器
	* int vxsize ： 地址计算方式中的参数320，即屏幕分辨率的x值
	* int pxsize, int pysize ： 图像的大小
	* int px0, int py0 ： 起始点x,y坐标（左上角）
	* char *buf ： 存储图像字符信息的指针
	* int bxsize ： 数组横向长度（这里使用一维数组模拟二维数组）
	*/
{
	int x, y;
	for (x = 0; x < pxsize; x++) {
		for (y = 0; y < pysize; y++) {
			vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
		}
	}

	return;
}

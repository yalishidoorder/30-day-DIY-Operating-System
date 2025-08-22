// 窗口相关

#include "my_bootpack.h"

// 绘制窗口标题内容
void make_wtitle8(unsigned char* buf, int xsize, char* title, char act)
    /* 参数含义
	* unsigned char* buf ： 保存图像信息
	* int xsize ： 屏幕显示参数
	* char* title ： 窗口标题
	* char act ： 窗口选中标志：0未选中，1选中
	*/
{
	static char closebtn[14][16] = {
		"OOOOOOOOOOOOOOO@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@",
		"@@@@@@@@@@@@@@@@"
	};
	int x, y;
	char c, tc, tbc;

	if (act != 0) {
		tc = COL8_FFFFFF;
		tbc = COL8_000084;
	}
	else {
		tc = COL8_C6C6C6;
		tbc = COL8_848484;
	}

	boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
	putfonts8_asc(buf, xsize, 24, 4, tc, title);

	for (y = 0; y < 14; y++) {
		for (x = 0; x < 16; x++) {
			c = closebtn[y][x];
			if (c == '@') {
				c = COL8_000000;
			}
			else if (c == '$') {
				c = COL8_848484;
			}
			else if (c == 'Q') {
				c = COL8_C6C6C6;
			}
			else {
				c = COL8_FFFFFF;
			}
			buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
		}
	}

	return;
}

// 绘画窗口剩余部分
void make_window8(unsigned char* buf, int xsize, int ysize, char* title, char act)
/* 参数含义
* unsigned char* buf ： 保存图像信息
* int xsize, int ysize ： 窗口大小
* char* title ： 窗口标题
* char act ： 窗口选中标志：0未选中，1选中
*/
{
	boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, xsize - 1, 0);
	boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, xsize - 2, 1);
	boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, 0, ysize - 1);
	boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, 1, ysize - 2);
	boxfill8(buf, xsize, COL8_848484, xsize - 2, 1, xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, xsize - 1, 0, xsize - 1, ysize - 1);
	boxfill8(buf, xsize, COL8_C6C6C6, 2, 2, xsize - 3, ysize - 3);
	boxfill8(buf, xsize, COL8_848484, 1, ysize - 2, xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, 0, ysize - 1, xsize - 1, ysize - 1);

	make_wtitle8(buf, xsize, title, act);

	return;
}

// 任一图层上字符的刷新
void putfonts8_asc_sht(struct SHEET* sht, int x, int y,
	int c, int b, char* s, int l)
	/* 参数含义
	* struct SHEET* sht ： 进行刷新的图层
	* int x, int y ： 刷新字符的x,y坐标
	* int c, int b ： 字符颜色和背景颜色
	* char* s ： 显示的字符串
	* int l ： 字符串长度
	*/
{
	struct TASK* task = task_now();
	boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15);
	// 全角
	if (task->langmode != 0 && task->langbyte1 != 0) { 
		putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
		sheet_refresh(sht, x - 8, y, x + l * 8, y + 16);
	}
	else {
		putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
		sheet_refresh(sht, x, y, x + l * 8, y + 16);
	}

	return;
}

// 绘制可输入文本框
void make_textbox8(struct SHEET* sht, int x0, int y0,
	int sx, int sy, int c)
	/* 参数含义
	* struct SHEET* sht ： 进行刷新的图层
	* int x0, int y0 ： 文本框左上角的x,y坐标
	* int sx, int sy ： 文本框长、宽
	* int c ： 文本框中央区间颜色
	*/
{
	int x1 = x0 + sx, y1 = y0 + sy;

	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, c, x0 - 1, y0 - 1, x1 + 0, y1 + 0);

	return;
}

// 改变窗口标题颜色
void change_wtitle8(struct SHEET* sht, char act)
    /* 参数含义
	* struct SHEET* sht ： 进行刷新的图层
	* char act ： 窗口选中标志：0未选中，1选中
	*/
{
	int x, y, xsize = sht->bxsize;
	char c, tc_new, tbc_new, tc_old, tbc_old, * buf = sht->buf;

	// 调整选中状态
	if (act != 0) {
		tc_new = COL8_FFFFFF;
		tbc_new = COL8_000084;
		tc_old = COL8_C6C6C6;
		tbc_old = COL8_848484;
	}
	else {
		tc_new = COL8_C6C6C6;
		tbc_new = COL8_848484;
		tc_old = COL8_FFFFFF;
		tbc_old = COL8_000084;
	}


	for (y = 3; y <= 20; y++) {
		for (x = 3; x <= xsize - 4; x++) {
			c = buf[y * xsize + x];
			if (c == tc_old && x <= xsize - 22) {
				c = tc_new;
			}
			else if (c == tbc_old) {
				c = tbc_new;
			}
			buf[y * xsize + x] = c;
		}
	}

	sheet_refresh(sht, 3, 3, xsize, 21);

	return;
}

/* bootpack的主要部分 */

#include <stdio.h>
#include "my_bootpack.h"

// 缓冲区大小
#define FIFO_BUF_S 128
#define KEYCMD_BUF_S 32

#define TASK_NUM        2
#define KEYCMD_LED      0xed

void set490(struct FIFO32* fifo, int mode);
void set_tss(struct TSS32 *tss);
void keywin_off(struct SHEET* key_win);
void keywin_on(struct SHEET* key_win);
void close_console(struct SHEET* sht);
void close_constask(struct TASK* task);

void HariMain(void)
{
	struct BOOTINFO* binfo = (struct BOOTINFO*)ADR_BOOTINFO;
	struct FIFO32 fifo, keycmd;
	// 变量显示、缓冲区
	char variable_display[40];
	int fifobuf[FIFO_BUF_S], keycmd_buf[KEYCMD_BUF_S];
	struct MOUSE_DEC mdec;

	// memman需要32KB，使用自0x003c0000开始的32KB
	unsigned int memtotal;
	struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;

	struct SHTCTL* shtctl;
	// 目前的图层：背景、鼠标指针、命令行的2个窗口
	struct SHEET* sht_back, * sht_mouse;
	unsigned char* buf_back, buf_mouse[256];
	struct TASK* task_a, * task;
	int j, x, y, mmx = -1, mmy = -1, mmx2 = 0;  // movemode
	struct SHEET* sht = 0, * key_win, * sht2;

	// 存放按下键的数值
	static char keytable0[0x80] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0x08, 0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0x0a, 0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
	};
	// 按下shift键
	static char keytable1[0x80] = {
		0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0x08, 0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0x0a, 0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
	};
	// key_shift : 0-null,1-l_shift,2-r_shift,3-b_shift
	// key_leds
	// keycmd_wait : 键盘将要写入的数据
	int key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;
	
	int* fat;
	unsigned char* nihongo;
	struct FILEINFO* finfo;
	extern char hankaku[4096];

	init_gdtidt();
	init_pic();
	io_sti();     // 开启中断

	fifo32_init(&fifo, FIFO_BUF_S, fifobuf, 0);
	fifo32_init(&keycmd, KEYCMD_BUF_S, keycmd_buf, 0);
	init_pit();
	init_keyboard(&fifo, KEY_S); //初始化键盘、鼠标
	enable_mouse(&fifo, MOUSE_S, &mdec);

	io_out8(PIC0_IMR, 0xf8); /* 允许PIC1和键盘(11111000) */
	io_out8(PIC1_IMR, 0xef); /* 允许鼠标(11101111) */

	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette(); //初始化调色板

	/* 使用指针从系统中取得参数值，不再写死 */
	// 对图层组和图层进行分配和初始化
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 2);
	*((int*)0x0fe4) = (int)shtctl;
	*((int*)0x0fec) = (int)&fifo;
	task_a->langmode = ENGLISH;

	/* sht_back */
	sht_back = sheet_alloc(shtctl);
	buf_back = (unsigned char*)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* 无透明色  */
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	
	/* sht_cons */
	key_win = open_console(shtctl, memtotal);                  

	/* sht_mouse */
	sht_mouse = sheet_alloc(shtctl);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	init_mouse_cursor8(buf_mouse, 99);
	int mx = (binfo->scrnx - MOUSE_SIZE) / 2; /* 计算坐标以使其位于画面中央 */
	int my = (binfo->scrny - 28 - MOUSE_SIZE) / 2;
	int  new_mx = -1, new_my = 0, new_wx = 0x7fffffff, new_wy = 0;

	sheet_slide(sht_back, 0, 0);
	sheet_slide(key_win, 32, 4);                   
	sheet_slide(sht_mouse, mx, my);
	// 初始化图层高度
	sheet_updown(sht_back, 0);     
	sheet_updown(key_win, 1);
	sheet_updown(sht_mouse, 2);          

	keywin_on(key_win);

	/* 为了避免和键盘当前状态冲突，在一开始先进行设置 */
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);

	/*载入nihongo.fnt */
	nihongo = (unsigned char*)memman_alloc_4k(memman, 16 * 256 + 32 * 94 * 47);
	fat = (int*)memman_alloc_4k(memman, 4 * 2880);
	file_readfat(fat, (unsigned char*)(ADR_DISKIMG + 0x000200));
	finfo = file_search("nihongo.fnt", (struct FILEINFO*)(ADR_DISKIMG + 0x002600), 224);
	int i;
	if (finfo != 0) {
		i = finfo->size;
		nihongo = file_loadfile2(finfo->clustno, &i, fat);
	}
	else {
		nihongo = (unsigned char*)memman_alloc_4k(memman, 
			16 * 256 + 32 * 94 * 47);
		for (i = 0; i < 16 * 256; i++) {
			nihongo[i] = hankaku[i]; /* 没有字库，半角部分直接复制英文字库 */
		}
		for (i = 16 * 256; i < 16 * 256 + 32 * 94 * 47; i++) {
			nihongo[i] = 0xff; /* 没有字库，全角部分以0xff填充 */
		}
	}
	*((int*)0x0fe8) = (int)nihongo;
	memman_free_4k(memman, (int)fat, 4 * 2880);

	while (1) {
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {   
			/*如果存在向键盘控制器发送的数据，则发送它  */
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		io_cli();
		if (fifo32_status(&fifo) == 0) {
			/* FIFO为空，当存在搁置的绘图操作时立即执行*/                
			if (new_mx >= 0) {
				io_sti();
				sheet_slide(sht_mouse, new_mx, new_my);
				new_mx = -1;
			}
			else if (new_wx != 0x7fffffff) {
				io_sti();
				sheet_slide(sht, new_wx, new_wy);
				new_wx = 0x7fffffff;
			}
			else {
				task_sleep(task_a);
				io_sti();
			}
		}
		else {
			int in_data = fifo32_get(&fifo);
			io_sti();
			/* 输入窗口被关闭 */
			if (key_win != 0 && key_win->flags == 0) {  
				/* 当画面上只剩鼠标和背景时 */
				if (shtctl->top == 1) { 
					key_win = 0;
				}
				else {
					key_win = shtctl->sheets[shtctl->top - 1];
					keywin_on(key_win);
				}                                                         
			}
			// 键盘数据
			if (KEY_S <= in_data && in_data < MOUSE_S) {
				
				/* 将按键编码转换为字符编码 */
				if (in_data < 0x80 + 256) { 
					if (key_shift == 0) {
						variable_display[0] = keytable0[in_data - 256];
					}
					else {
						variable_display[0] = keytable1[in_data - 256];
					}
				}
				else {
					variable_display[0] = 0;
				}

				/* 当输入字符为英文字母时 */
				if ('A' <= variable_display[0] && variable_display[0] <= 'Z') {   
					if (((key_leds & 4) == 0 && key_shift == 0) ||
						((key_leds & 4) != 0 && key_shift != 0)) {
						variable_display[0] += 0x20;   /*将大写字母转换为小写字母*/
					}
				}
				/* 一般字符、退格键、回车键 */
				if (variable_display[0] != 0) { 
					fifo32_put(&key_win->task->fifo, variable_display[0] + 256);
				}
				/* Tab键*/
				if (in_data == 256 + 0x0f && key_win != 0) {
					keywin_off(key_win);
					j = key_win->height - 1;
					if (j == 0) {
						j = shtctl->top - 1;
					}
					key_win = shtctl->sheets[j];
					keywin_on(key_win);
				}
				/* F11-将最下层窗口调整至第二层 */
				else if (in_data == 256 + 0x57 && shtctl->top > 2) {    
					sheet_updown(shtctl->sheets[1], shtctl->top - 1);
					/*到此结束*/
				}
				/* 左Shift ON */
				else if (in_data == 256 + 0x2a) {
					key_shift |= 1;
				}
				/* 右Shift ON */
				else if (in_data == 256 + 0x36) {
					key_shift |= 2;
				}
				/* 左Shift OFF */
				else if (in_data == 256 + 0xaa) {
					key_shift &= ~1;
				}
				/* 右Shift OFF */
				else if (in_data == 256 + 0xb6) {
					key_shift &= ~2;
				}
				/* CapsLock */
				else if (in_data == 256 + 0x3a) {
					key_leds ^= 4;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				/* NumLock */
				else if (in_data == 256 + 0x45) {
					key_leds ^= 2;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				/* ScrollLock */
				else if (in_data == 256 + 0x46) {
					key_leds ^= 1;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				/* Shift+F1 强行终止任务*/
				else if (in_data == 256 + 0x3b && key_shift != 0 && key_win != 0) {
					task = key_win->task;
					if (task != 0 && task->tss.ss0 != 0) {  /* Shift+F1 */
						cons_putstr0(task->cons, "\nBreak(key) :\n");
						io_cli();   /*强制结束处理时禁止任务切换*/
						task->tss.eax = (int)&(task->tss.esp0);
						task->tss.eip = (int)asm_end_app;
						io_sti();
						/* 为了确实执行结束处理，如果处于休眠状态则唤醒 */
						task_run(task, -1, 0);
					}
				}
				/* Shift+F2 打开一个新的命令行窗口 */
				else if (in_data == 256 + 0x3c && key_shift != 0) {	
					if (key_win != 0) {
						keywin_off(key_win);
					}
					key_win = open_console(shtctl, memtotal);
					/* 自动将输入焦点切换到新打开的命令行窗口 */
					sheet_slide(key_win, 32, 4);
					sheet_updown(key_win, shtctl->top);
					keywin_on(key_win);
				}
				/* 键盘成功接收到数据 */
				else if (in_data == 256 + 0xfa) {
					keycmd_wait = -1;
				}
				/* 键盘没有成功接收到数据 */
				else if (in_data == 256 + 0xfe) {
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}
			}
			// 鼠标数据
			else if (MOUSE_S <= in_data && in_data <= 767) {
				if (mouse_decode(&mdec, in_data - 512) != 0) {
					
					/* 移动鼠标指针 */
					mx += mdec.x;
					my += mdec.y;

					/* 限定鼠标指针坐标范围 */
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 1) {
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1) {
						my = binfo->scrny - 1;
					}

					new_mx = mx;                                             
					new_my = my;
					//sprintf(variable_display, "(mouse_height:%d)", sht_mouse->height);
					//putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_00FFFF, variable_display, 16);
					// 按下左键
					if ((mdec.btn & 0x01) != 0) {
						if (mmx < 0) {
							/*如果处于通常模式*/
							/*按照从上到下的顺序寻找鼠标所指向的图层*/
							for (j = shtctl->top - 1; j > 0; j--) {
								sht = shtctl->sheets[j];
								x = mx - sht->vx0;
								y = my - sht->vy0;
								if (0 <= x && x < sht->bxsize 
									&& 0 <= y && y < sht->bysize) {
									if (sht->buf[y * sht->bxsize + x] != 
										sht->col_inv) {
										sheet_updown(sht, shtctl->top - 1);

										if (sht != key_win) {
											keywin_off(key_win);
											key_win = sht;
											keywin_on(key_win);
										}
										/* 进入窗口移动模式 */
										if (3 <= x && x < sht->bxsize - 3 
											&& 3 <= y && y < 21) {
											mmx = mx;  
											mmy = my;
											mmx2 = sht->vx0;
											new_wy = sht->vy0;
										}
										/* 点击“×”按钮 */
										if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 
											&& 5 <= y && y < 19) {
											/* 该窗口为应用程序窗口 */
											if ((sht->flags & 0x10) != 0) {
												task = sht->task;
												cons_putstr0(task->cons, "\nBreak(mouse) :\n");
												/* 强制结束处理中禁止切换任务 */
												io_cli();	
												task->tss.eax = (int)&(task->tss.esp0);
												task->tss.eip = (int)asm_end_app;
												io_sti();
												task_run(task, -1, 0);
											}
											/* 命令行窗口 */
											else {    
												task = sht->task;
												/* 暂且隐藏该图层 */
												sheet_updown(sht, -1); 
												keywin_off(key_win);
												key_win = shtctl->sheets[shtctl->top - 1];
												keywin_on(key_win);
												io_cli();
												fifo32_put(&task->fifo, EXIT);
												io_sti();
											}
										}

										break;
									}
								}
							}
						}
						else {
							/*如果处于窗口移动模式*/
							x = mx - mmx;   /*计算鼠标的移动距离*/
							y = my - mmy;
							new_wx = (mmx2 + x + 2) & ~3;                      
							new_wy = new_wy + y;
							/*更新为移动后的坐标*/
							mmy = my;
						}
					}
					else {
						/*没有按下左键*/
						mmx = -1;   /*返回通常模式*/
						if (new_wx != 0x7fffffff) {              
							sheet_slide(sht, new_wx, new_wy);   /*固定图层位置*/
							new_wx = 0x7fffffff;
						}
					}
				}
			}
			/* 命令行窗口关闭处理 */
			else if (768 <= in_data && in_data <= 1023) {           
				close_console(shtctl->sheets0 + (in_data - 768));
			}
			else if (1024 <= in_data && in_data <= 2023) {
				close_constask(taskctl->tasks0 + (in_data - 1024));
			}
			/* 只关闭命令行窗口 */
			else if (CLOSE_CONS_WIN <= in_data && in_data <= 2279) {             
				sht2 = shtctl->sheets0 + (in_data - CLOSE_CONS_WIN);
				memman_free_4k(memman, (int)sht2->buf, 256 * 165);
				sheet_free(sht2);
			}
		}
	}
}

// 设定50天后才超时的490个定时器
void set490(struct FIFO32* fifo, int mode)
{
	int i;
	struct TIMER* timer;

	if (mode != 0) {
		for (i = 0; i < 490; i++) {
			timer = timer_alloc();
			timer_init(timer, fifo, 1024 + i);
			// 50天后，每隔1秒就有一个定时器超时
			timer_settime(timer, 60 * 60 * 24 * 50 * TIPS + i * TIPS);
		}
	}

	return;
}

// 设定任务b的相关参数
void set_tss(struct TSS32 *tss)
{
	tss->es = 1 * 8;
	tss->cs = 2 * 8;
	tss->ss = 1 * 8;
	tss->ds = 1 * 8;
	tss->fs = 1 * 8;
	tss->gs = 1 * 8;

	return;
}

// 关闭当前窗口输入
void keywin_off(struct SHEET* key_win)
    /* 参数含义
	* struct SHEET* key_win ： 当前窗口图层
	*/
{
	change_wtitle8(key_win, 0);

	if ((key_win->flags & 0x20) != 0) {
		fifo32_put(&key_win->task->fifo, FLASHING_OFF); /* 命令行窗口光标OFF */
	}
}

// 开启当前窗口输入
void keywin_on(struct SHEET* key_win)
    /* 参数含义
	* struct SHEET* key_win ： 当前窗口图层
	*/
{
	change_wtitle8(key_win, 1);

	if ((key_win->flags & 0x20) != 0) {
		fifo32_put(&key_win->task->fifo, FLASHING_ON); /* 命令行窗口光标ON */
	}
}

// 开启任务
struct TASK* open_constask(struct SHEET* sht, unsigned int memtotal)
{
	struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
	struct TASK* task = task_alloc();
	int* cons_fifo = (int*)memman_alloc_4k(memman, 128 * 4);

	task->cons_stack = memman_alloc_4k(memman, 64 * 1024);
	// 任务的栈，两个参数大小都为4字节，第一个参数地址是esp+4，所以-12
	task->tss.esp = task->cons_stack + 64 * 1024 - 12;
	task->tss.eip = (int)&console_task;
	set_tss(&task->tss);
	*((int*)(task->tss.esp + 4)) = (int)sht;
	*((int*)(task->tss.esp + 8)) = memtotal;
	task_run(task, 2, 2); /* level=2, priority=2 */

	fifo32_init(&task->fifo, 128, cons_fifo, task);

	return task;
}

// 打开命令行窗口
struct SHEET* open_console(struct SHTCTL* shtctl, unsigned int memtotal)
{
	struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
	struct SHEET* sht = sheet_alloc(shtctl);
	unsigned char* buf = (unsigned char*)memman_alloc_4k(memman, 256 * 165);

	sheet_setbuf(sht, buf, 256, 165, -1); /*无透明色*/
	make_window8(buf, 256, 165, "console", 0);
	make_textbox8(sht, 8, 28, 240, 128, COL8_000000);
	
	sht->task = open_constask(sht, memtotal);
	sht->flags |= 0x20; /*有光标*/

	return sht;
}

// 结束任务
void close_constask(struct TASK* task)
{
	struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
	task_sleep(task);
	memman_free_4k(memman, task->cons_stack, 64 * 1024);
	memman_free_4k(memman, (int)task->fifo.buf, 128 * 4);
	task->flags = 0; /*用来替代task_free(task); */
	return;
}

// 释放窗口资源
void close_console(struct SHEET* sht)
{
	struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
	struct TASK* task = sht->task;
	memman_free_4k(memman, (int)sht->buf, 256 * 165);
	sheet_free(sht);
	close_constask(task);
	return;
}

    /*
	* 画条纹
	int i;
	char *p;
	for (i = 0; i <= 0xffff; i++) {
		p[i] = i & 0x0f;    // equal to *(p + i) = i & 0x0f;
		// 也可以使用c语言指针进阶知识，进行强制类型转换
	}
	*/

	/*
	* 画矩形
	boxfill8(p, 320, COL8_FF0000, 20, 20, 120, 120);
	boxfill8(p, 320, COL8_00FF00, 70, 50, 170, 150);
	boxfill8(p, 320, COL8_0000FF, 120, 80, 220, 180);
	*/

	/*
	* 提示词
	putfonts8_asc(binfo->vram, binfo->scrnx, 8, 8, COL8_FFFFFF, "ABC 123");
	putfonts8_asc(binfo->vram, binfo->scrnx, 31, 31, COL8_000000, "WorldTrade WEI OS.");
	putfonts8_asc(binfo->vram, binfo->scrnx, 30, 30, COL8_FFFFFF, "WorldTrade WEI OS.");
	*/

	/*
	* 打印变量的值
	char s[40];
	sprintf(s, "scrnx = %d", binfo->scrnx);
	putfonts8_asc(binfo->vram, binfo->scrnx, 30, 30, COL8_FFFFFF, s);
	*/





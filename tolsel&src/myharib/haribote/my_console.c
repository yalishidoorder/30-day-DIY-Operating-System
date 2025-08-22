// 命令行相关

#include <stdio.h>
#include <string.h>
#include "my_bootpack.h"

// 执行命令行任务
void console_task(struct SHEET* sheet, unsigned int memtotal)
{
	struct TASK* task = task_now();
	struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
	struct FILEHANDLE fhandle[8];
	unsigned char* nihongo = (char*)*((int*)0x0fe8);
	int * fat = (int*)memman_alloc_4k(memman, 4 * 2880); 
	char cmdline[30];
	struct CONSOLE cons;

	cons.sht = sheet;
	cons.cur_x = 8;
	cons.cur_y = 28;
	cons.cur_c = -1;
	task->cons = &cons;
	task->cmdline = cmdline;
	
	if (cons.sht != 0) {
		cons.timer = timer_alloc();
		timer_init(cons.timer, &task->fifo, 1);
		timer_settime(cons.timer, 0.5 * TIPS);
	}

	file_readfat(fat, (unsigned char*)(ADR_DISKIMG + 0x000200));

	int i;
	for (i = 0; i < 8; i++) {
		fhandle[i].buf = 0; /*未使用标记*/
	}
	task->fhandle = fhandle;
	task->fat = fat;
	if (nihongo[4096] != 0xff) {    /*是否载入了日文字库？*/
		task->langmode = NIHONGO;
	}
	else {
		task->langmode = ENGLISH;
	}
	task->langbyte1 = 0;

	/*显示提示符*/
	cons_putchar(&cons, '>', 1);

	while (1) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		}
		else {
			int data = fifo32_get(&task->fifo);
			io_sti();
			/* 光标用定时器 */
			if (data <= 1 && cons.sht != 0) {
				if (data != 0) {
					timer_init(cons.timer, &task->fifo, 0); /*下次置0 */
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_FFFFFF;
					}
				}
				else {
					timer_init(cons.timer, &task->fifo, 1); /*下次置1 */
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_000000;
					}
				}
				timer_settime(cons.timer, 0.5 * TIPS);
			}
			/* 光标ON */
			else if (data == FLASHING_ON) {
				cons.cur_c = COL8_FFFFFF;
			}
			/* 光标OFF */
			else if (data == FLASHING_OFF) {
				if (cons.sht != 0) {
					boxfill8(cons.sht->buf, cons.sht->bxsize, COL8_000000,
						cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				cons.cur_c = -1;
			}
			/* 点击命令行窗口的“×”按钮 */
			else if (data == EXIT) {         
				cmd_exit(&cons, fat);
			}
			/* 键盘数据（通过任务A） */
			else if (KEY_S <= data && data < MOUSE_S) {
				if (data == BACKSPACE + 256) {
					/*退格键*/
					if (cons.cur_x > 16) {
						/*用空白擦除光标后将光标前移一位*/
						if (cons.cur_x < 248) {
							cons_putchar(&cons, ' ', 0);
						}

						cons.cur_x -= 8;
					}
				}
				else if (data == ENTER + 256) {
					/*回车键*/
					/*用空格将光标擦除*/
					cons_putchar(&cons, ' ', 0);
					cmdline[cons.cur_x / 8 - 2] = 0;
					cons_newline(&cons);
					cons_runcmd(cmdline, &cons, fat, memtotal); /*运行命令*/
					if (cons.sht == 0) {                  
						cmd_exit(&cons, fat);
					}
					/*显示提示符*/
					cons_putchar(&cons, '>', 1);
				}
				else {
					/*一般字符*/
					if (cons.cur_x < 248) {
						/*显示一个字符之后将光标后移一位  */
						cmdline[cons.cur_x / 8 - 2] = data - 256;
						cons_putchar(&cons, data - 256, 1);
					}
				}
			}

			/* 重新显示光标 */
			if (cons.sht != 0) {
				if (cons.cur_x < 248 && cons.cur_c >= 0) {
					boxfill8(cons.sht->buf, cons.sht->bxsize, cons.cur_c,
						cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				sheet_refresh(cons.sht, cons.cur_x, 28, cons.cur_x + 8, cons.cur_y + 16);
			}
		}
	}
}

// 输出单个字符
void cons_putchar(struct CONSOLE* cons, int chr, char move)
    /* 参数含义
	* struct CONSOLE* cons ： 
	* int chr ： 字符的ascii码
	* char move ： 换行标志
	*/
{
	char c[2];
	c[0] = chr;
	c[1] = 0;
	if (c[0] == 0x09) { /*制表符*/
		while (1) {
			if (cons->sht != 0) {
				putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y,
					COL8_FFFFFF, COL8_000000, " ", 1);
			}
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 240) {
				cons_newline(cons);
			}
			if (((cons->cur_x - 8) & 0x1f) == 0) {
				break;  /* 被32整除则break */
			}
		}
	}
	/* 换行 */
	else if (c[0] == 0x0a) { 
		cons_newline(cons);
	}
	/* 回车 */
	else if (c[0] == 0x0d) {  
		/**/
	}
	/* 一般字符 */
	else {    
		if (cons->sht != 0) {
			putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, 
				COL8_FFFFFF, COL8_000000, c, 1);
		}

		if (move != 0) {
			/* move为0时光标不后移 */
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 240) {
				cons_newline(cons);
			}
		}
	}

	return;
}

// 换行函数
void cons_newline(struct CONSOLE* cons)
{
	int x, y;
	struct SHEET* sheet = cons->sht;
	struct TASK* task = task_now();
	if (cons->cur_y < 28 + 112) {
		cons->cur_y += 16; 
	}
	/* 滚动 */
	else if (sheet != 0) {
		for (y = 28; y < 28 + 112; y++) {
			for (x = 8; x < 8 + 240; x++) {
				sheet->buf[x + y * sheet->bxsize] =
					sheet->buf[x + (y + 16) * sheet->bxsize];
			}
		}
		for (y = 28 + 112; y < 28 + 128; y++) {
			for (x = 8; x < 8 + 240; x++) {
				sheet->buf[x + y * sheet->bxsize] = COL8_000000;
			}
		}
		sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
	}
	cons->cur_x = 8;

	// 第一个全角字符换行
	if (task->langmode == NIHONGO && task->langbyte1 != 0) {      
		cons->cur_x += 8;
	}

	return;
}

void cons_putstr0(struct CONSOLE* cons, char* s)
{
	for (; *s != 0; s++) {
		cons_putchar(cons, *s, 1);
	}
	return;
}

void cons_putstr1(struct CONSOLE* cons, char* s, int l)
{
	int i;
	for (i = 0; i < l; i++) {
		cons_putchar(cons, s[i], 1);
	}
	return;
}

void cons_runcmd(char* cmdline, struct CONSOLE* cons, int* fat, unsigned int memtotal)
    /* 参数含义
	* char* cmdline ： 在命令行输入的内容
	* struct CONSOLE* cons ：
	* int* fat ： 文件分配表
	* unsigned int memtotal ： 内存大小
	*/
{
	if (strcmp(cmdline, "mem") == 0) {
		cmd_mem(cons, memtotal);
	}
	else if (strcmp(cmdline, "cls") == 0) {
		cmd_cls(cons);
	}
	else if (strcmp(cmdline, "dir") == 0) {
		cmd_dir(cons);
	}
	else if (strcmp(cmdline, "exit") == 0) {
		cmd_exit(cons, fat);
	}
	else if (strncmp(cmdline, "start ", 6) == 0) {
		cmd_start(cons, cmdline, memtotal);
	}
	else if (strncmp(cmdline, "ncst ", 5) == 0) {                     
		cmd_ncst(cons, cmdline, memtotal);                          
	}
	else if (strncmp(cmdline, "langmode ", 9) == 0) {
		cmd_langmode(cons, cmdline);
	}
	else if (cmdline[0] != 0) {
		if (cmd_app(cons, fat, cmdline) == 0) {
			/* 不是命令，也不是空行 */
			cons_putstr0(cons, "Invalid command.\n\n");
		}
	}

	return;
}

// mem命令
void cmd_mem(struct CONSOLE* cons, unsigned int memtotal)
{
	char variable_display[61];
	struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;

	sprintf(variable_display, "total   %dMB\nfree %dKB\n\n", 
		memtotal / (1024 * 1024), memman_total(memman) / 1024);
	cons_putstr0(cons, variable_display);

	return;
}

// cls命令
void cmd_cls(struct CONSOLE* cons)
{
	int x, y;
	struct SHEET* sheet = cons->sht;

	for (y = 28; y < 28 + 128; y++) {
		for (x = 8; x < 8 + 240; x++) {
			sheet->buf[x + y * sheet->bxsize] = COL8_000000;
		}
	}
	sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
	cons->cur_y = 28;
}

// dir命令
void cmd_dir(struct CONSOLE* cons) 
{
	struct FILEINFO* finfo = (struct FILEINFO*)(ADR_DISKIMG + 0x002600);
	char variable_display[30];
	int x, y;

	for (x = 0; x < 224; x++) {
		// 不包含文件名信息
		if (finfo[x].name[0] == 0x00) {
			break;
		}
		// 文件存在
		if (finfo[x].name[0] != 0xe5) {
			if ((finfo[x].type & 0x18) == 0) {
				sprintf(variable_display,
					"filename.ext   %7d\n", finfo[x].size);
				for (y = 0; y < 8; y++) {
					variable_display[y] = finfo[x].name[y];
				}
				variable_display[9] = finfo[x].ext[0];
				variable_display[10] = finfo[x].ext[1];
				variable_display[11] = finfo[x].ext[2];
				cons_putstr0(cons, variable_display);
			}
		}
	}
	cons_newline(cons);
	return;
}

// exit命令
void cmd_exit(struct CONSOLE* cons, int* fat)
{
	struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
	struct TASK* task = task_now();
	struct SHTCTL* shtctl = (struct SHTCTL*)*((int*)0x0fe4);
	struct FIFO32* fifo = (struct FIFO32*)*((int*)0x0fec);

	if (cons->sht != 0) {
		timer_cancel(cons->timer);
	}
	memman_free_4k(memman, (int)fat, 4 * 2880);
	io_cli();
	if (cons->sht != 0) {           /*从此开始*/
		fifo32_put(fifo, cons->sht - shtctl->sheets0 + START_CMD);    /* 768～1023 */
	}
	else {
		fifo32_put(fifo, task - taskctl->tasks0 + NCST_CMD);    /*1024～2023*/
	}
	io_sti();
	while (1) {
		task_sleep(task);
	}
}

// start命令
void cmd_start(struct CONSOLE* cons, char* cmdline, int memtotal)
{
	struct SHTCTL* shtctl = (struct SHTCTL*)*((int*)0x0fe4);
	struct SHEET* sht = open_console(shtctl, memtotal);
	struct FIFO32* fifo = &sht->task->fifo;
	int i;
	sheet_slide(sht, 32, 4);
	sheet_updown(sht, shtctl->top);

	/*将命令行输入的字符串逐字复制到新的命令行窗口中*/
	for (i = 6; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, ENTER + 256); /*回车键*/
	cons_newline(cons);

	return;
}

// ncst命令
void cmd_ncst(struct CONSOLE* cons, char* cmdline, int memtotal)
{
	struct TASK* task = open_constask(0, memtotal);
	struct FIFO32* fifo = &task->fifo;
	int i;

	/*将命令行输入的字符串逐字复制到新的命令行窗口中*/
	for (i = 5; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256); /*回车键*/
	cons_newline(cons);
	return;
}

// langmode命令
void cmd_langmode(struct CONSOLE* cons, char* cmdline)
{
	struct TASK* task = task_now();
	unsigned char mode = cmdline[9] - '0';

	if (mode <= EUC) {
		task->langmode = mode;
		char variable_display[13];
		sprintf(variable_display, "langmode:%d\n", mode);
		cons_putstr0(cons, variable_display);
	}
	else {
		cons_putstr0(cons, "mode number error.\n");
	}
	cons_newline(cons);
	return;
}

int cmd_app(struct CONSOLE* cons, int* fat, char* cmdline)
{
	struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
	struct FILEINFO* finfo;
	char name[18], * p, * q;
	struct TASK* task = task_now();
	int i, segsiz, datsiz, esp, dathrb, appsiz;
	struct SHTCTL* shtctl;  
	struct SHEET* sht;

	/*根据命令行生成文件名*/
	for (i = 0; i < 13; i++) {
		if (cmdline[i] <= ' ') {
			break;
		}
		name[i] = cmdline[i];
	}
	name[i] = 0; /*暂且将文件名的后面置为0*/

	/*寻找文件 */
	finfo = file_search(name, (struct FILEINFO*)(ADR_DISKIMG + 0x002600), 224);
	if (finfo == 0 && name[i - 1] != '.') {
		/*由于找不到文件，故在文件名后面加上“.hrb”后重新寻找*/
		name[i] = '.';
		name[i + 1] = 'H';
		name[i + 2] = 'R';
		name[i + 3] = 'B';
		name[i + 4] = 0;
		finfo = file_search(name, (struct FILEINFO*)(ADR_DISKIMG + 0x002600), 224);
	}

	if (finfo != 0) {
		/*找到文件的情况*/
		appsiz = finfo->size;
		/* 分配程序专用的内存空间 */
		// 代码段
		p = file_loadfile2(finfo->clustno, &appsiz, fat);
		if (finfo->size >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
			segsiz = *((int*)(p + 0x0000));
			esp = *((int*)(p + 0x000c));
			datsiz = *((int*)(p + 0x0010));
			dathrb = *((int*)(p + 0x0014));
			// 数据段
			q = (char*)memman_alloc_4k(memman, segsiz);
			task->ds_base = (int)q;
			
			set_segmdesc(task->ldt + 0, finfo->size - 1, (int)p, AR_CODE32_ER + 0x60);
			set_segmdesc(task->ldt + 1, segsiz - 1, (int)q, AR_DATA32_RW + 0x60);

			for (i = 0; i < datsiz; i++) {
				q[esp + i] = p[dathrb + i];
			}
			start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));
			shtctl = (struct SHTCTL*)*((int*)0x0fe4);       
			for (i = 0; i < MAX_SHEETS; i++) {
				sht = &(shtctl->sheets0[i]);
				/* 找到被应用程序遗留的窗口 */
				if ((sht->flags & 0x11) == 0x11 && sht->task == task) {
					sheet_free(sht);    /*关闭*/
				}
			}
			/* 将未关闭的文件关闭 */
			for (i = 0; i < 8; i++) {
				if (task->fhandle[i].buf != 0) {
					memman_free_4k(memman, (int)task->fhandle[i].buf, 
						task->fhandle[i].size);
					task->fhandle[i].buf = 0;
				}
			}
			timer_cancelall(&task->fifo); // 取消定时器
			memman_free_4k(memman, (int)q, segsiz);
			task->langbyte1 = 0;
		}
		else {
			cons_putstr0(cons, ".hrb file format error.\n");
		}
		memman_free_4k(memman, (int)p, appsiz);
		cons_newline(cons);
		return 1;
	}
	/*没有找到文件的情况*/
	return 0;
}

int* hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax)
{
	struct TASK* task = task_now();
	int ds_base = task->ds_base;
	struct CONSOLE* cons = task->cons;
	struct SHTCTL* shtctl = (struct SHTCTL*)*((int*)0x0fe4);
	struct FIFO32* sys_fifo = (struct FIFO32*)*((int*)0x0fec);
	struct SHEET* sht;
	int* reg = &eax + 1;	 /* eax后面的地址 */
	/* 强行改写通过PUSHAD保存的值 */
	/* reg[0] : EDI,   reg[1] : ESI,   reg[2] : EBP,   reg[3] : ESP */
	/* reg[4] : EBX,   reg[5] : EDX,   reg[6] : ECX,   reg[7] : EAX */
	int data, i;
	struct FILEINFO* finfo;
	struct FILEHANDLE* fh;
	struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;

	if (edx == 1) {
		cons_putchar(cons, eax & 0xff, 1);
	}
	else if (edx == 2) {
		cons_putstr0(cons, (char*)ebx + ds_base);
	}
	else if (edx == 3) {
		cons_putstr1(cons, (char*)ebx + ds_base, ecx);
	}
	else if (edx == 4) {
		return &(task->tss.esp0);
	}
	// 绘制窗口
	else if (edx == 5) {
		sht = sheet_alloc(shtctl);
		sht->task = task;
		sht->flags |= 0x10;
		sheet_setbuf(sht, (char*)ebx + ds_base, esi, edi, eax);
		make_window8((char*)ebx + ds_base, esi, edi, (char*)ecx + ds_base, 0);
		sheet_slide(sht, ((shtctl->xsize - esi) / 2) & ~3, (shtctl->ysize - edi) / 2);
		/* 将窗口图层高度指定为当前鼠标所在图层的高度，鼠标移到上层 */
		sheet_updown(sht, shtctl->top);
		reg[7] = (int)sht;
	}
	// 窗口字体
	else if (edx == 6) {
		sht = (struct SHEET*)(ebx & 0xfffffffe);
		putfonts8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char*)ebp + ds_base);
		if ((ebx & 1) == 0) {
			sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
		}
	}
	// 窗口方块
	else if (edx == 7) {
		sht = (struct SHEET*)(ebx & 0xfffffffe);
		boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
		if ((ebx & 1) == 0) {
			sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
		}
	}
	else if (edx == 8) {
		memman_init((struct MEMMAN*)(ebx + ds_base));
		ecx &= 0xfffffff0;  /* 以16字节为单位 */
		memman_free((struct MEMMAN*)(ebx + ds_base), eax, ecx);
	}
	else if (edx == 9) {
		ecx = (ecx + 0x0f) & 0xfffffff0; /* 以16字节为单位进位取整 */
		reg[7] = memman_alloc((struct MEMMAN*)(ebx + ds_base), ecx);
	}
	else if (edx == 10) {
		ecx = (ecx + 0x0f) & 0xfffffff0; /* 以16字节为单位进位取整 */
		memman_free((struct MEMMAN*)(ebx + ds_base), eax, ecx);
	}
	// 画点
	else if (edx == 11) {
		sht = (struct SHEET*)(ebx & 0xfffffffe);
		sht->buf[sht->bxsize * edi + esi] = eax;
		if ((ebx & 1) == 0) {
			sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
		}
	}
	// 刷新窗口
	else if (edx == 12) {
		sht = (struct SHEET*)ebx;
		sheet_refresh(sht, eax, ecx, esi, edi);
	}
	else if (edx == 13) {
		sht = (struct SHEET*)(ebx & 0xfffffffe);
		hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
		if ((ebx & 1) == 0) {
			if (eax > esi) {
				i = eax;
				eax = esi;
				esi = i;
			}
			if (ecx > edi) {
				i = ecx;
				ecx = edi;
				edi = i;
			}                      
			sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
		}
	}
	// 关闭窗口
	else if (edx == 14) {
		sheet_free((struct SHEET*)ebx);
	}
	else if (edx == 15) {
		while (1) {
			io_cli();
			if (fifo32_status(&task->fifo) == 0) {
				if (eax != 0) {
					task_sleep(task);   /* FIFO为空，休眠并等待*/
				}
				else {
					io_sti();
					reg[7] = -1;
					return 0;
				}
			}
			data = fifo32_get(&task->fifo);
			io_sti();
			if (data <= 1) { /*光标用定时器*/
				/* 应用程序运行时不需要显示光标，因此总是将下次显示用的值置为1 */
				timer_init(cons->timer, &task->fifo, 1); /* 下次置为1 */
				timer_settime(cons->timer, 50);
			}
			/* 光标ON */
			if (data == FLASHING_ON) {
				cons->cur_c = COL8_FFFFFF;
			}
			/* 光标OFF */
			if (data == FLASHING_OFF) {
				cons->cur_c = -1;
			}
			/* 只关闭命令行窗口 */
			if (data == 4) {
				timer_cancel(cons->timer);
				io_cli();
				/*2024～2279*/
				fifo32_put(sys_fifo, cons->sht - shtctl->sheets0 + CLOSE_CONS_WIN);
				cons->sht = 0;
				io_sti();
			}
			/*键盘数据（通过任务A）*/
			if (data >= KEY_S) {
				reg[7] = data - KEY_S;
				return 0;
			}
		}
	}
	else if (edx == 16) {
		reg[7] = (int)timer_alloc();
		((struct TIMER*)reg[7])->flags2 = AUTO_SHUTDOWN;
	}
	else if (edx == 17) {
		timer_init((struct TIMER*)ebx, &task->fifo, eax + 256);
	}
	else if (edx == 18) {
		timer_settime((struct TIMER*)ebx, eax);
	}
	else if (edx == 19) {
		timer_free((struct TIMER*)ebx);
	}
	else if (edx == 20) {
		if (eax == 0) {
			data = io_in8(0x61);
			io_out8(0x61, data & 0x0d);
		}
		else {
			data = 1193180000 / eax;
			io_out8(0x43, 0xb6);
			io_out8(0x42, data & 0xff);
			io_out8(0x42, data >> 8);
			data = io_in8(0x61);
			io_out8(0x61, (data | 0x03) & 0x0f);
		}
	}
	// 打开文件
	else if (edx == 21) {
		for (i = 0; i < 8; i++) {
			if (task->fhandle[i].buf == 0) {
				break;
			}
		}
		fh = &task->fhandle[i];
		reg[7] = 0;
		if (i < 8) {
			finfo = file_search((char*)ebx + ds_base,
				(struct FILEINFO*)(ADR_DISKIMG + 0x002600), 224);
			if (finfo != 0) {
				reg[7] = (int)fh;
				fh->buf = (char*)memman_alloc_4k(memman, finfo->size);
				fh->size = finfo->size;
				fh->pos = 0;
				fh->buf = file_loadfile2(finfo->clustno, &fh->size, task->fat);
			}
		}
	}
	// 关闭文件
	else if (edx == 22) {
		fh = (struct FILEHANDLE*)eax;
		memman_free_4k(memman, (int)fh->buf, fh->size);
		fh->buf = 0;
	}
	// 文件定位
	else if (edx == 23) {
		fh = (struct FILEHANDLE*)eax;
		if (ecx == 0) {
			fh->pos = ebx;
		}
		else if (ecx == 1) {
			fh->pos += ebx;
		}
		else if (ecx == 2) {
			fh->pos = fh->size + ebx;
		}
		if (fh->pos < 0) {
			fh->pos = 0;
		}
		if (fh->pos > fh->size) {
			fh->pos = fh->size;
		}
	}
	// 获取文件大小
	else if (edx == 24) {
		fh = (struct FILEHANDLE*)eax;
		if (ecx == 0) {
			reg[7] = fh->size;
		}
		else if (ecx == 1) {
			reg[7] = fh->pos;
		}
		else if (ecx == 2) {
			reg[7] = fh->pos - fh->size;
		}
	}
	// 文件读取
	else if (edx == 25) {
		fh = (struct FILEHANDLE*)eax;
		for (i = 0; i < ecx; i++) {
			if (fh->pos == fh->size) {
				break;
			}
			*((char*)ebx + ds_base + i) = fh->buf[fh->pos];
			fh->pos++;
		}
		reg[7] = i;
	}
	// 获取命令行
	else if (edx == 26) {
		i = 0;
		for (;;) {
			*((char*)ebx + ds_base + i) = task->cmdline[i];
			if (task->cmdline[i] == 0) {
				break;
			}
			if (i >= ecx) {
				break;
			}
			i++;
		}
		reg[7] = i;
		}
	// 获取langmode
	else if (edx == 27) {
		reg[7] = task->langmode;
	}
	return 0;
}

int* inthandler0c(int* esp)
{
	struct TASK* task = task_now();
	struct CONSOLE* cons = task->cons;
	char s[30];     

	cons_putstr0(cons, "\nINT 0C :\n Stack Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);    
	cons_putstr0(cons, s);                 

	return &(task->tss.esp0);   /*强制结束程序*/
}

int* inthandler0d(int* esp)
{
	struct TASK* task = task_now();
	struct CONSOLE* cons = task->cons;
	char s[30];     

	cons_putstr0(cons, "\nINT 0D :\n General Protected Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);    
	cons_putstr0(cons, s);                 

	return &(task->tss.esp0);   /*强制结束程序*/
}

// 画线函数
void hrb_api_linewin(struct SHEET* sht, 
	int x0, int y0, int x1, int y1, int col)
    /* 参数含义
	* struct SHEET* sht ：
	* int x0, int y0 ： 起始点坐标
	* int x1, int y1 ： 终点坐标
	* int col ： 颜色
	*/
{
	int i, x, y, len, dx, dy;

	dx = x1 - x0;
	dy = y1 - y0;
	x = x0 << 10;
	y = y0 << 10;

	if (dx < 0) {
		dx = -dx;
	}
	if (dy < 0) {
		dy = -dy;
	}
	if (dx >= dy) {
		len = dx + 1;
		if (x0 > x1) {
			dx = -1024;
		}
		else {
			dx = 1024;
		}
		if (y0 <= y1) {
			dy = ((y1 - y0 + 1) << 10) / len;
		}
		else {
			dy = ((y1 - y0 - 1) << 10) / len;
		}
	}
	else {
		len = dy + 1;
		if (y0 > y1) {
			dy = -1024;
		}
		else {
			dy = 1024;
		}
		if (x0 <= x1) {
			dx = ((x1 - x0 + 1) << 10) / len;
		}
		else {
			dx = ((x1 - x0 - 1) << 10) / len;
		}
	}

	for (i = 0; i < len; i++) {
		sht->buf[(y >> 10) * sht->bxsize + (x >> 10)] = col;
		x += dx;
		y += dy;
	}

	return;
}

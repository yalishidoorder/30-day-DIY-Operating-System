/* asmhead.nas */
struct BOOTINFO { /* 0x0ff0-0x0fff */
	char cyls; /* 启动区读硬盘读到何处为止 */
	char leds; /* 启动时键盘LED的状态 */
	char vmode; /* 显卡模式为多少位彩色 */
	char reserve;
	short scrnx, scrny; /* 画面分辨率 */
	char* vram;
};

#define ADR_BOOTINFO	0x00000ff0
#define ADR_DISKIMG     0x00100000 

/* naskfunc.nas */
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
int load_cr0(void);
void store_cr0(int cr0);
void load_tr(int tr);
void asm_inthandler0c(void);
void asm_inthandler0d(void);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);
void farjmp(int eip, int cs);
void farcall(int eip, int cs);
void asm_hrb_api(void);
void start_app(int eip, int cs, int esp, int ds, int* tss_esp0);
void asm_end_app(void);

/* fifo.c */
/*
int* buf : 改良为32位
int size : 缓冲区总字节数
int free : 缓冲区无数据的字节数
int n_w : 下一个数据写入地址
int n_r : 下一个数据读出地址
struct TASK *task : 记录要唤醒任务的信息
*/
struct FIFO32 
{
	int* buf;
	int n_w, n_r, size, free, flags;
	struct TASK* task;
};
void fifo32_init(struct FIFO32* fifo, int size, int* buf, struct TASK* task);
int fifo32_put(struct FIFO32* fifo, int data);
int fifo32_get(struct FIFO32* fifo);
int fifo32_status(struct FIFO32* fifo);

/* graphic.c */
void init_palette(void);
void set_palette(int start, int end, unsigned char* rgb);
void boxfill8(unsigned char* vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(char* vram, int x, int y);
void putfont8(char* vram, int xsize, int x, int y, char c, char* font);
void putfonts8_asc(char* vram, int xsize, int x, int y, char c, unsigned char* s);
void init_mouse_cursor8(char* mouse, char bc);
void putblock8_8(char* vram, int vxsize, int pxsize,
	int pysize, int px0, int py0, char* buf, int bxsize);
// 定义颜色号
#define COL8_000000		0 /*  0:黑 */
#define COL8_FF0000		1 /*  1:亮红 */
#define COL8_00FF00		2 /*  2:亮绿 */
#define COL8_FFFF00		3 /*  3:亮黄 */
#define COL8_0000FF		4 /*  4:亮蓝 */
#define COL8_FF00FF		5 /*  5:亮紫 */
#define COL8_00FFFF		6 /*  6:浅亮蓝 */
#define COL8_FFFFFF		7 /*  7:白 */
#define COL8_C6C6C6		8 /*  8:亮灰 */
#define COL8_840000		9 /*  9:暗红 */
#define COL8_008400		10/* 10:暗绿 */
#define COL8_848400		11/* 11:暗黄 */
#define COL8_000084		12/* 12:暗青 */
#define COL8_840084		13/* 13:暗紫 */
#define COL8_008484		14/* 14:浅暗蓝 */
#define COL8_848484		15/* 15:暗灰 */

#define length 16 /* 点阵数组长度 */
#define MOUSE_SIZE 16 /* 鼠标大小 */

/* dsctbl.c */
// 保存全局段号记录表的8字节内容
struct SEGMENT_DESCRIPTOR {
	short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
};
// 保存中断记录表
struct GATE_DESCRIPTOR {
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
};
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR* sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR* gd, int offset, int selector, int ar);
#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_LDT          0x0082
#define AR_TSS32		0x0089
#define AR_INTGATE32	0x008e

/* int.c */
void init_pic(void);
void inthandler27(int* esp);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

/* keyboard.c */
void inthandler21(int* esp);
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32* fifo, int data0);
#define PORT_KEYDAT		0x0060
#define PORT_KEYCMD		0x0064

/* mouse.c */
struct MOUSE_DEC 
{
	unsigned char buf[3], phase; // 鼠标键值的三字节内容，阶段标志位
	// 鼠标键值第一字节区分鼠标移动和鼠标点击，后两字节分别为左右，上下移动信息
	int x, y, btn; // 存放移动信息和鼠标按键状态
};
void inthandler2c(int* esp);
void enable_mouse(struct FIFO32* fifo, int data0, struct MOUSE_DEC* mdec);
int mouse_decode(struct MOUSE_DEC* mdec, unsigned char dat);

/* memory.c */
#define MEMMAN_FREES		4090	/* 大约是32KB */
#define MEMMAN_ADDR			0x003c0000

/* 可用信息 */
struct FREEINFO
{
	unsigned int addr, size;
};
/* 内存管理 */
struct MEMMAN
{
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN* man);
unsigned int memman_total(struct MEMMAN* man);
unsigned int memman_alloc(struct MEMMAN* man, unsigned int size);
int memman_free(struct MEMMAN* man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN* man, unsigned int size);
int memman_free_4k(struct MEMMAN* man, unsigned int addr, unsigned int size);

/* sheet.c */
#define MAX_SHEETS		256

struct SHEET
{
	unsigned char* buf; // 记录绘制内容地址
	// 图层大小、位置坐标、透明色色号、图层高度、设定信息
	int bxsize, bysize, vx0, vy0, col_inv, height, flags;
	struct SHTCTL* ctl; // 图层所属的图层组
	struct TASK* task;
};
struct SHTCTL
{
	unsigned char* vram, * map; // 图像数据随机存储器地址和保存画面上像素点属于的图层的容器
	int xsize, ysize, top;  // VRAM画面的大小、最高层图层高度
	struct SHEET* sheets[MAX_SHEETS];  // 按高度升序保存图层地址
	struct SHEET sheets0[MAX_SHEETS];  // 保存图层信息
};
struct SHTCTL* shtctl_init(struct MEMMAN* memman, unsigned char* vram, int xsize, int ysize);
struct SHEET* sheet_alloc(struct SHTCTL* ctl);
void sheet_setbuf(struct SHEET* sht, unsigned char* buf, int xsize, int ysize, int col_inv);
void sheet_setbuf(struct SHEET* sht, unsigned char* buf, int xsize, int ysize, int col_inv);
void sheet_refreshmap(struct SHTCTL* ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);
void sheet_refreshsub(struct SHTCTL* ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);
void sheet_updown(struct SHEET* sht, int height);
void sheet_refresh(struct SHEET* sht, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct SHEET* sht, int vx0, int vy0);
void sheet_free(struct SHEET* sht);

/* timer.c */
#define MAX_TIMER   500  
// 1s发生100次定时器中断
#define TIPS 100

#define AUTO_SHUTDOWN 1
#define NO_SHUTDOWN   0
struct TIMER
{
	struct TIMER* next_timer;    // 下一个定时器地址
	unsigned int timeout;        // 定时
	char flags, flags2;          // 使用状态和自动关闭状态
	struct FIFO32* fifo;
	int data;
};
struct TIMERCTL 
{
	// 记录定时器中断的次数、下一个定时器结束的时刻
	unsigned int count, next_time;
	struct TIMER* t_head;  // 定时器链表头指针
	struct TIMER timers0[MAX_TIMER]; // 存放定时器信息
};

extern struct TIMERCTL timerctl;
void init_pit(void);
struct TIMER* timer_alloc(void);
void timer_free(struct TIMER* timer);
void timer_init(struct TIMER* timer, struct FIFO32* fifo, int data);
void timer_settime(struct TIMER* timer, unsigned int timeout);
void timer_remove(struct TIMER* timer);
void timer_insert(struct TIMER* timer);
void inthandler20(int* esp);
int timer_cancel(struct TIMER* timer);
void timer_cancelall(struct FIFO32* fifo);

/* mtask.c */
#define MAX_TASKS       1000   /* 最大任务数量 */
#define TASK_GDT0       3      /* 定义从GDT的几号开始分配给TSS */
#define TASK_SLEEPING   1      /* 非工作状态 */
#define TASK_RUNNING    2      /* 工作状态 */
#define MAX_TASKS_LV    100    /* 每个级别最大任务数量 */
#define MAX_TASKLEVELS  10     /* 任务等级范围为0-9 */

// 32位任务状态段
struct TSS32
{
	// 任务相关
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
	// 32位reg
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	// 16位reg
	int es, cs, ss, ds, fs, gs;
	// 任务相关，iomap置为0x40000000
	int ldtr, iomap;
};
struct TASK 
{
	int sel, flags;  /* sel用来存放GDT的编号*/
	int level, priority;   // 优先级和切换时间
	struct FIFO32 fifo;
	struct TSS32 tss;
	struct SEGMENT_DESCRIPTOR ldt[2];
	struct CONSOLE* cons;
	int ds_base, cons_stack;
	struct FILEHANDLE* fhandle;     // 文件句柄
	int* fat;
	char* cmdline;
	unsigned char langmode, langbyte1;        // 语言模式，存放首字节数据
};
struct TASKLEVEL 
{
	int running; /* 正在运行的任务数量 */
	int now;     /* 记录当前正在运行的是哪个任务 */
	struct TASK* tasks[MAX_TASKS_LV];
};
struct TASKCTL 
{
	int now_lv; /*现在活动中的LEVEL */
	char lv_change; /*在下次任务切换时是否需要改变LEVEL */
	struct TASKLEVEL level[MAX_TASKLEVELS];
	struct TASK tasks0[MAX_TASKS];
};
extern struct TIMER* task_timer;
extern struct TASKCTL* taskctl;
struct TASK* task_init(struct MEMMAN* memman);
struct TASK* task_alloc(void);
void task_run(struct TASK* task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK* task);
struct TASK* task_now(void);
void task_add(struct TASK* task);
void task_remove(struct TASK* task);

/* window.c */
void make_window8(unsigned char* buf, int xsize, int ysize, char* title, char act);
void putfonts8_asc_sht(struct SHEET* sht, int x, int y, int c, int b, char* s, int l);
void make_textbox8(struct SHEET* sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle8(unsigned char* buf, int xsize, char* title, char act);
void change_wtitle8(struct SHEET* sht, char act);

/* console.c */
// 键盘输入值范围是256-511，偏移量是256
#define KEY_S             256
// 鼠标输入值范围是512-767，偏移量是512
#define MOUSE_S           512
// start指令数据范围768-1023，偏移量是768
#define START_CMD         768 
// ncst指令数据范围1024-2023，偏移量是1024
#define NCST_CMD          1024
// 关闭命令行窗口数据范围2024-2279，偏移量是2024
#define CLOSE_CONS_WIN    2024
// 退格键ascii码为8
#define BACKSPACE 8
// 回车键ascii码为10
#define ENTER 10

// console光标开始闪烁定义为2
#define FLASHING_ON 2
// 停止闪烁定义为3
#define FLASHING_OFF  3
// 退出指令
#define EXIT           4

struct CONSOLE 
{
	struct SHEET* sht;
	int cur_x, cur_y, cur_c;
	struct TIMER* timer;
};
struct FILEHANDLE 
{                 
	char* buf;
	int size;
	int pos;
};
void console_task(struct SHEET* sheet, unsigned int memtotal);
void cons_putchar(struct CONSOLE* cons, int chr, char move);
void cons_newline(struct CONSOLE* cons);
void cons_putstr0(struct CONSOLE* cons, char* s);
void cons_putstr1(struct CONSOLE* cons, char* s, int l);
void cons_runcmd(char* cmdline, struct CONSOLE* cons, int* fat, unsigned int memtotal);
void cmd_mem(struct CONSOLE* cons, unsigned int memtotal);
void cmd_cls(struct CONSOLE* cons);
void cmd_dir(struct CONSOLE* cons);
void cmd_exit(struct CONSOLE* cons, int* fat);
void cmd_start(struct CONSOLE* cons, char* cmdline, int memtotal);
void cmd_ncst(struct CONSOLE* cons, char* cmdline, int memtotal);
void cmd_langmode(struct CONSOLE* cons, char* cmdline);
int cmd_app(struct CONSOLE* cons, int* fat, char* cmdline);
int* hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);
int* inthandler0d(int* esp);
int* inthandler0c(int* esp);
void hrb_api_linewin(struct SHEET* sht, int x0, int y0, int x1, int y1, int col);

/* file.c */
struct FILEINFO
{
	// 文件名 扩展名 属性
	unsigned char name[8], ext[3], type;
	// 保留
	char reserve[10];
	// 扇区号
	unsigned short time, date, clustno;
	unsigned int size;
};
void file_readfat(int* fat, unsigned char* img);
void file_loadfile(int clustno, int size, char* buf, int* fat, char* img);
struct FILEINFO* file_search(char* name, struct FILEINFO* finfo, int max);
char* file_loadfile2(int clustno, int* psize, int* fat);

/* bootpack.c */
#define ENGLISH    0
#define NIHONGO    1
#define EUC		   2
#define CHINESE    3
struct TASK* open_constask(struct SHEET* sht, unsigned int memtotal);
struct SHEET* open_console(struct SHTCTL* shtctl, unsigned int memtotal);

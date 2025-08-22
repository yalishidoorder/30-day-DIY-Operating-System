// 文件关系

#include <string.h>
#include "my_bootpack.h"

/* 将磁盘映像中的FAT解压缩 */
void file_readfat(int* fat, unsigned char* img)
/* 参数含义
* int* fat ： 文件分配表
* unsigned char* img ： 映像文件
*/
{
	int i, j = 0;
	for (i = 0; i < 2880; i += 2) {
		fat[i + 0] = (img[j + 0] | img[j + 1] << 8) & 0xfff;
		fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
		j += 3;
	}

	return;
}

void file_loadfile(int clustno, int size, char* buf, int* fat, char* img)
/* 参数含义
* int clustno ： 扇区号
* int size ： 文件大小
* char* buf ： buffer，缓冲区
* int* fat ： 文件分配表
* unsigned char* img ： 映像文件
*/
{
	int i;
	for (;;) {
		if (size <= 512) {
			for (i = 0; i < size; i++) {
				buf[i] = img[clustno * 512 + i];
			}
			break;
		}
		for (i = 0; i < 512; i++) {
			buf[i] = img[clustno * 512 + i];
		}
		size -= 512;
		buf += 512;
		clustno = fat[clustno];
	}
	return;
}

// 查找文件
struct FILEINFO* file_search(char* name, struct FILEINFO* finfo, int max)
    /* 参数含义
    * char* name ： 文件名
    * struct FILEINFO* finfo ：
    * int max ： 最大搜索长度
    */
{
    int i, j;
    char s[12];

    for (j = 0; j < 11; j++) {
        s[j] = ' ';
    }
    j = 0;

    for (i = 0; name[i] != 0; i++) {
        if (j >= 11) { 
            return 0; /*没有找到*/ 
        
        }

        if (name[i] == '.' && j <= 8) {
            j = 8;
        }
        else {
            s[j] = name[i];
            if ('a' <= s[j] && s[j] <= 'z') {
                /*将小写字母转换为大写字母*/
                s[j] -= 0x20;
            }
            j++;
        }
    }

    for (i = 0; i < max; ) {
        if (finfo[i].name[0] == 0x00) {
            break;
        }
        if ((finfo[i].type & 0x18) == 0) {
            if (strncmp(finfo[i].name, s, 11) == 0) {
                return finfo + i; /*找到文件*/
            }
            else {
                i++;
            }
        }
    }

    return 0; /*没有找到*/
}

char* file_loadfile2(int clustno, int* psize, int* fat)
{
    int size = *psize, size2;
    struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
    char* buf, * buf2;

    buf = (char*)memman_alloc_4k(memman, size);
    file_loadfile(clustno, size, buf, fat, (char*)(ADR_DISKIMG + 0x003e00));
    // tek格式有一个用于识别格式的文件头
    if (size >= 17) {
        size2 = tek_getsize(buf);
        if (size2 > 0) {    /*使用tek格式压缩的文件*/
            buf2 = (char*)memman_alloc_4k(memman, size2);
            tek_decomp(buf, buf2, size2);
            memman_free_4k(memman, (int)buf, size);
            buf = buf2;
            *psize = size2;
        }
    }
    return buf;
}

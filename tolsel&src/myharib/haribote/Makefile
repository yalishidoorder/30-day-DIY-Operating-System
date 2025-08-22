OBJS_BOOTPACK = my_bootpack.obj naskfunc.obj hankaku.obj my_graphic.obj my_dsctbl.obj \
		my_int.obj my_fifo.obj my_keyboard.obj my_mouse.obj my_memory.obj my_sheet.obj \
                my_timer.obj my_mtask.obj my_console.obj my_file.obj my_window.obj tek.obj

TOOLPATH = ../../z_tools/
INCPATH  = ../../z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = ../haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
GOLIB    = $(TOOLPATH)golib00.exe 
COPY     = copy
DEL      = del

# 僨僼僅儖僩摦嶌

default :
	$(MAKE) ipl09.bin
	$(MAKE) haribote.sys

# 僼傽僀儖惗惉婯懃

ipl09.bin : ipl09.nas Makefile
	$(NASK) ipl09.nas ipl09.bin ipl09.lst

asmhead.bin : asmhead.nas Makefile
	$(NASK) asmhead.nas asmhead.bin asmhead.lst

hankaku.bin : hankaku.txt Makefile
	$(MAKEFONT) hankaku.txt hankaku.bin

hankaku.obj : hankaku.bin Makefile
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku

my_bootpack.bim : $(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:my_bootpack.bim stack:3136k map:my_bootpack.map \
		$(OBJS_BOOTPACK)
# 3MB+64KB=3136KB

my_bootpack.hrb : my_bootpack.bim Makefile
	$(BIM2HRB) my_bootpack.bim my_bootpack.hrb 0

haribote.sys : asmhead.bin my_bootpack.hrb Makefile
	copy /B asmhead.bin+my_bootpack.hrb haribote.sys

# 堦斒婯懃

%.gas : %.c Makefile
	$(CC1) -o $*.gas $*.c

%.nas : %.gas Makefile
	$(GAS2NASK) $*.gas $*.nas

%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst


# 僐儅儞僪

clean :
	-$(DEL) asmhead.bin
	-$(DEL) hankaku.bin
	-$(DEL) *.lst
	-$(DEL) *.obj
	-$(DEL) *.map
	-$(DEL) *.bim
	-$(DEL) *.hrb

src_only :
	$(MAKE) clean
	-$(DEL) ipl09.bin
	-$(DEL) haribote.sys

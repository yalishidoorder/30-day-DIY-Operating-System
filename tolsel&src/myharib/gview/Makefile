APP1      = my_gview
APP2      = gview
STACK    = 4480k
MALLOC   = 0k

TOOLPATH = ../../z_tools/
INCPATH  = ../../z_tools/haribote/
APILIBPATH   = ../apilib/
HARIBOTEPATH = ../haribote/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -I../ -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
BIM2BIN  = $(TOOLPATH)bim2bin.exe
RULEFILE = ../haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
GOLIB    = $(TOOLPATH)golib00.exe 
COPY     = copy
DEL      = del

# 默认动作
default :
	$(MAKE) $(APP2).hrb

# 文件生成规则

$(APP1).bim : $(APP1).obj bmp.obj jpeg.obj $(APILIBPATH)apilib.lib Makefile
	$(OBJ2BIM) @$(RULEFILE) out:$(APP1).bim map:$(APP).map stack:$(STACK) \
		$(APP1).obj jpeg.obj bmp.obj $(APILIBPATH)apilib.lib

$(APP1).org : $(APP1).bim Makefile
	$(BIM2HRB) $(APP1).bim $(APP1).org $(MALLOC)

$(APP2).hrb : $(APP1).org Makefile
	$(BIM2BIN) -osacmp in:$(APP1).org out:$(APP2).hrb

haribote.img : ../haribote/ipl20.bin ../haribote/haribote.sys $(APP2).hrb \
		Makefile
	$(EDIMG)   imgin:../../z_tools/fdimg0at.tek \
		wbinimg src:../haribote/ipl20.bin len:512 from:0 to:0 \
		copy from:../haribote/haribote.sys to:@: \
		copy from:$(APP2).hrb to:@: \
		copy from:../nihongo/nihongo.fnt to:@: \
		imgout:haribote.img

# 一般规则
%.gas : %.c ../my_apilib.h Makefile
	$(CC1) -o $*.gas $*.c

%.nas : %.gas Makefile
	$(GAS2NASK) $*.gas $*.nas

%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst

%.org : %.bim Makefile
	$(BIM2HRB) $*.bim $*.org $(MALLOC)

%.hrb : %.org Makefile
	$(BIM2BIN) -osacmp in:$*.org out:$*.hrb

# 命令
run :
	$(MAKE) haribote.img
	$(COPY) haribote.img ..\..\z_tools\qemu\fdimage0.bin
	$(MAKE) -C ../../z_tools/qemu

full :
	$(MAKE) -C $(APILIBPATH)
	$(MAKE) $(APP2).hrb

run_full :
	$(MAKE) -C $(APILIBPATH)
	$(MAKE) -C ../haribote
	$(MAKE) run

clean :
	-$(DEL) *.lst
	-$(DEL) $(APP1).obj  # 修改这里，使用 APP1 而不是硬编码
	-$(DEL) jpeg.obj
	-$(DEL) *.map
	-$(DEL) *.bim
	-$(DEL) *.org
	-$(DEL) haribote.img

src_only :
	$(MAKE) clean
	-$(DEL) $(APP2).hrb
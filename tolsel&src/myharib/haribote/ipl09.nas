; haribote-ipl
; TAB=4

CYLS	EQU		9				; 要读取多少内容

		ORG		0x7c00			; 偙偺僾儘僌儔儉偑偳偙偵撉傒崬傑傟傞偺偐

; 埲壓偼昗弨揑側FAT12僼僅乕儅僢僩僼儘僢僺乕僨傿僗僋偺偨傔偺婰弎

		JMP		entry
		DB		0x90
		DB		"HARIBOTE"		; 僽乕僩僙僋僞偺柤慜傪帺桼偵彂偄偰傛偄乮8僶僀僩乯
		DW		512				; 1僙僋僞偺戝偒偝乮512偵偟側偗傟偽偄偗側偄乯
		DB		1				; 僋儔僗僞偺戝偒偝乮1僙僋僞偵偟側偗傟偽偄偗側偄乯
		DW		1				; FAT偑偳偙偐傜巒傑傞偐乮晛捠偼1僙僋僞栚偐傜偵偡傞乯
		DB		2				; FAT偺屄悢乮2偵偟側偗傟偽偄偗側偄乯
		DW		224				; 儖乕僩僨傿儗僋僩儕椞堟偺戝偒偝乮晛捠偼224僄儞僩儕偵偡傞乯
		DW		2880			; 偙偺僪儔僀僽偺戝偒偝乮2880僙僋僞偵偟側偗傟偽偄偗側偄乯
		DB		0xf0			; 儊僨傿傾偺僞僀僾乮0xf0偵偟側偗傟偽偄偗側偄乯
		DW		9				; FAT椞堟偺挿偝乮9僙僋僞偵偟側偗傟偽偄偗側偄乯
		DW		18				; 1僩儔僢僋偵偄偔偮偺僙僋僞偑偁傞偐乮18偵偟側偗傟偽偄偗側偄乯
		DW		2				; 僿僢僪偺悢乮2偵偟側偗傟偽偄偗側偄乯
		DD		0				; 僷乕僥傿僔儑儞傪巊偭偰側偄偺偱偙偙偼昁偢0
		DD		2880			; 偙偺僪儔僀僽戝偒偝傪傕偆堦搙彂偔
		DB		0,0,0x29		; 傛偔傢偐傜側偄偗偳偙偺抣偵偟偰偍偔偲偄偄傜偟偄
		DD		0xffffffff		; 偨傇傫儃儕儏乕儉僔儕傾儖斣崋
		DB		"HARIBOTEOS "	; 僨傿僗僋偺柤慜乮11僶僀僩乯
		DB		"FAT12   "		; 僼僅乕儅僢僩偺柤慜乮8僶僀僩乯
		RESB	18				; 偲傝偁偊偢18僶僀僩偁偗偰偍偔

; 僾儘僌儔儉杮懱

entry:
		MOV		AX,0			; 儗僕僗僞弶婜壔
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX

; 读取磁盘

		MOV		AX,0x0820
		MOV		ES,AX
		MOV     	CH,0            		; 0柱面
                MOV     	DH,0           		 ; 0磁头
        	MOV     	CL,2            		; 2扇区
        	MOV     	BX, 18*2*CYLS-1  ; 要读取的合计扇区数                 
		CALL    	readfast        ; 告诉读取

; 读取结束，运行haribote.sys！

		MOV		BYTE [0x0ff0],CYLS	;  记录IPL实际读取了多少内容  
		JMP		0xc200

error:
		MOV		AX,0
		MOV		ES,AX
		MOV		SI,msg
putloop:
		MOV		AL,[SI]
		ADD		SI,1			; 将SI加1
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; 显示一个字符的函数
		MOV		BX,15			; 颜色代码
		INT		0x10			; 调用显示BIOS
		JMP		putloop
fin:
		HLT						; 暂时让CPU停止运行
		JMP		fin				; 无限循环
msg:
		DB		0x0a, 0x0a		;  两个换行
		DB		"load error"
		DB		0x0a			; 换行
		DB		0

readfast:	 ; 使用AL尽量一次性读取数据 从此开始
;   ES:读取地址, CH:柱面, DH:磁头, CL:扇区, BX:读取扇区数

		MOV		AX,ES			 ; < 通过ES计算AL的最大值 >
        	SHL     	AX,3            ; 将AX除以32，将结果存入AH（SHL是左移位指令）
        	AND     	AH,0x7f         ; AH是AH除以128所得的余数（512*128=64K）
		MOV		AL,128			;  AL = 128 - AH; AH是AH除以128所得的余数 （512*128=64K）
		SUB		AL,AH

		MOV		AH,BL			;  < 通过BX计算AL的最大值并存入AH >
		CMP		BH,0			;  if (BH != 0) { AH = 18; }
		JE		.skip1
		MOV		AH,18
.skip1:
		CMP		AL,AH			; if (AL > AH) { AL = AH; }
		JBE		.skip2
		MOV		AL,AH
.skip2:

		MOV		AH,19			; < 通过CL计算AL的最大值并存入AH  >
		SUB		AH,CL			; AH = 19 - CL;
		CMP		AL,AH			; if (AL > AH) { AL = AH; }
		JBE		.skip3
		MOV		AL,AH
.skip3:

		PUSH	BX
		MOV		SI,0			; 计算失败次数的寄存器
retry:
		MOV		AH,0x02			; AH=0x02 : 读取磁盘
		MOV		BX,0
		MOV		DL,0x00			; A盘
		PUSH	ES
		PUSH	DX
		PUSH	CX
		PUSH	AX
		INT		0x13			; 调用磁盘BIOS
        	JNC     	next            ; 没有出错的话则跳转至next
        	ADD     	SI,1            ; 将SI加1
        	CMP     	SI,5            ; 将SI与5比较
        	JAE     	error           ; SI >= 5则跳转至error
        	MOV     	AH,0x00
        	MOV     	DL,0x00         ; A盘
        	INT     	0x13            ; 驱动器重置
		POP		AX
		POP		CX
		POP		DX
		POP		ES
		JMP		retry
next:
		POP		AX
		POP		CX
		POP		DX
		POP		BX				 ; 将ES的内容存入BX
        	SHR     	BX,5            ; 将BX由16字节为单位转换为512字节为单位
        	MOV     	AH,0
       		ADD     	BX,AX           ; BX += AL;
        	SHL     	BX,5            ; 将BX由512字节为单位转换为16字节为单位
        	MOV     	ES,BX           ; 相当于EX += AL * 0x20;
        	POP     	BX
        	SUB     	BX,AX
        	JZ      	.ret
        	ADD     	CL,AL           ; 将CL加上AL
        	CMP     	CL,18           ; 将CL与18比较
        	JBE     	readfast        ; CL <= 18则跳转至readfast
        	MOV     	CL,1
        	ADD     	DH,1
        	CMP     	DH,2
        	JB      	readfast        ; DH < 2则跳转至readfast
		MOV		DH,0
		ADD		CH,1
		JMP		readfast
.ret:
		RET

		RESB	0x7dfe-$		; 到0x7dfe为止用0x00填充的指令

		DB		0x55, 0xaa

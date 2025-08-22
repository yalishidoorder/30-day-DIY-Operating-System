; haribote-ipl
; TAB=4

CYLS	EQU		9				; Ҫ��ȡ��������

		ORG		0x7c00			; ���̃v���O�������ǂ��ɓǂݍ��܂��̂�

; �ȉ��͕W���I��FAT12�t�H�[�}�b�g�t���b�s�[�f�B�X�N�̂��߂̋L�q

		JMP		entry
		DB		0x90
		DB		"HARIBOTE"		; �u�[�g�Z�N�^�̖��O�����R�ɏ����Ă悢�i8�o�C�g�j
		DW		512				; 1�Z�N�^�̑傫���i512�ɂ��Ȃ���΂����Ȃ��j
		DB		1				; �N���X�^�̑傫���i1�Z�N�^�ɂ��Ȃ���΂����Ȃ��j
		DW		1				; FAT���ǂ�����n�܂邩�i���ʂ�1�Z�N�^�ڂ���ɂ���j
		DB		2				; FAT�̌��i2�ɂ��Ȃ���΂����Ȃ��j
		DW		224				; ���[�g�f�B���N�g���̈�̑傫���i���ʂ�224�G���g���ɂ���j
		DW		2880			; ���̃h���C�u�̑傫���i2880�Z�N�^�ɂ��Ȃ���΂����Ȃ��j
		DB		0xf0			; ���f�B�A�̃^�C�v�i0xf0�ɂ��Ȃ���΂����Ȃ��j
		DW		9				; FAT�̈�̒����i9�Z�N�^�ɂ��Ȃ���΂����Ȃ��j
		DW		18				; 1�g���b�N�ɂ����̃Z�N�^�����邩�i18�ɂ��Ȃ���΂����Ȃ��j
		DW		2				; �w�b�h�̐��i2�ɂ��Ȃ���΂����Ȃ��j
		DD		0				; �p�[�e�B�V�������g���ĂȂ��̂ł����͕K��0
		DD		2880			; ���̃h���C�u�傫����������x����
		DB		0,0,0x29		; �悭�킩��Ȃ����ǂ��̒l�ɂ��Ă����Ƃ����炵��
		DD		0xffffffff		; ���Ԃ�{�����[���V���A���ԍ�
		DB		"HARIBOTEOS "	; �f�B�X�N�̖��O�i11�o�C�g�j
		DB		"FAT12   "		; �t�H�[�}�b�g�̖��O�i8�o�C�g�j
		RESB	18				; �Ƃ肠����18�o�C�g�����Ă���

; �v���O�����{��

entry:
		MOV		AX,0			; ���W�X�^������
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX

; ��ȡ����

		MOV		AX,0x0820
		MOV		ES,AX
		MOV     	CH,0            		; 0����
                MOV     	DH,0           		 ; 0��ͷ
        	MOV     	CL,2            		; 2����
        	MOV     	BX, 18*2*CYLS-1  ; Ҫ��ȡ�ĺϼ�������                 
		CALL    	readfast        ; ���߶�ȡ

; ��ȡ����������haribote.sys��

		MOV		BYTE [0x0ff0],CYLS	;  ��¼IPLʵ�ʶ�ȡ�˶�������  
		JMP		0xc200

error:
		MOV		AX,0
		MOV		ES,AX
		MOV		SI,msg
putloop:
		MOV		AL,[SI]
		ADD		SI,1			; ��SI��1
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; ��ʾһ���ַ��ĺ���
		MOV		BX,15			; ��ɫ����
		INT		0x10			; ������ʾBIOS
		JMP		putloop
fin:
		HLT						; ��ʱ��CPUֹͣ����
		JMP		fin				; ����ѭ��
msg:
		DB		0x0a, 0x0a		;  ��������
		DB		"load error"
		DB		0x0a			; ����
		DB		0

readfast:	 ; ʹ��AL����һ���Զ�ȡ���� �Ӵ˿�ʼ
;   ES:��ȡ��ַ, CH:����, DH:��ͷ, CL:����, BX:��ȡ������

		MOV		AX,ES			 ; < ͨ��ES����AL�����ֵ >
        	SHL     	AX,3            ; ��AX����32�����������AH��SHL������λָ�
        	AND     	AH,0x7f         ; AH��AH����128���õ�������512*128=64K��
		MOV		AL,128			;  AL = 128 - AH; AH��AH����128���õ����� ��512*128=64K��
		SUB		AL,AH

		MOV		AH,BL			;  < ͨ��BX����AL�����ֵ������AH >
		CMP		BH,0			;  if (BH != 0) { AH = 18; }
		JE		.skip1
		MOV		AH,18
.skip1:
		CMP		AL,AH			; if (AL > AH) { AL = AH; }
		JBE		.skip2
		MOV		AL,AH
.skip2:

		MOV		AH,19			; < ͨ��CL����AL�����ֵ������AH  >
		SUB		AH,CL			; AH = 19 - CL;
		CMP		AL,AH			; if (AL > AH) { AL = AH; }
		JBE		.skip3
		MOV		AL,AH
.skip3:

		PUSH	BX
		MOV		SI,0			; ����ʧ�ܴ����ļĴ���
retry:
		MOV		AH,0x02			; AH=0x02 : ��ȡ����
		MOV		BX,0
		MOV		DL,0x00			; A��
		PUSH	ES
		PUSH	DX
		PUSH	CX
		PUSH	AX
		INT		0x13			; ���ô���BIOS
        	JNC     	next            ; û�г���Ļ�����ת��next
        	ADD     	SI,1            ; ��SI��1
        	CMP     	SI,5            ; ��SI��5�Ƚ�
        	JAE     	error           ; SI >= 5����ת��error
        	MOV     	AH,0x00
        	MOV     	DL,0x00         ; A��
        	INT     	0x13            ; ����������
		POP		AX
		POP		CX
		POP		DX
		POP		ES
		JMP		retry
next:
		POP		AX
		POP		CX
		POP		DX
		POP		BX				 ; ��ES�����ݴ���BX
        	SHR     	BX,5            ; ��BX��16�ֽ�Ϊ��λת��Ϊ512�ֽ�Ϊ��λ
        	MOV     	AH,0
       		ADD     	BX,AX           ; BX += AL;
        	SHL     	BX,5            ; ��BX��512�ֽ�Ϊ��λת��Ϊ16�ֽ�Ϊ��λ
        	MOV     	ES,BX           ; �൱��EX += AL * 0x20;
        	POP     	BX
        	SUB     	BX,AX
        	JZ      	.ret
        	ADD     	CL,AL           ; ��CL����AL
        	CMP     	CL,18           ; ��CL��18�Ƚ�
        	JBE     	readfast        ; CL <= 18����ת��readfast
        	MOV     	CL,1
        	ADD     	DH,1
        	CMP     	DH,2
        	JB      	readfast        ; DH < 2����ת��readfast
		MOV		DH,0
		ADD		CH,1
		JMP		readfast
.ret:
		RET

		RESB	0x7dfe-$		; ��0x7dfeΪֹ��0x00����ָ��

		DB		0x55, 0xaa

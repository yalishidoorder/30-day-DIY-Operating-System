APP1      = hello5
APP2      = hello5
STACK    = 1k
MALLOC   = 0k

include ../app_make.txt

$(APP2).hrb : $(APP1).org Makefile
	$(COPY) $(APP1).org $(APP2).hrb

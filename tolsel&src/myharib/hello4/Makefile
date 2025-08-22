APP1      = my_hello4
APP2      = hello4
STACK    = 1k
MALLOC   = 0k

include ../app_make.txt

$(APP2).hrb : $(APP1).org Makefile
	$(COPY) $(APP1).org $(APP2).hrb

APP1      = my_winhelo
APP2      = winhelo
STACK    = 8k
MALLOC   = 0k

include ../app_make.txt

$(APP2).hrb : $(APP1).org Makefile
	$(COPY) $(APP1).org $(APP2).hrb

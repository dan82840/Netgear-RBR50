#
#	Copyright (C) 2016 Delta Networks Inc.
#


CC = gcc

FILES := main.o soap.o socket.o rpc.o util.o info.o
FILES2 := agent.o util.o
FILES3 := satelliteinfo.o util.o
FILES4 := check_fw.o 

CFLAGS += -Wall -Wunused -g -O2

LDFLAGS += -lconfig

all : soapclient soap_agent satelliteinfo soap_check_fw

soapclient : $(FILES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(FILES) -o $@
	@echo "make $@ finished on `date`"

soap_agent : $(FILES2)
	$(CC) $(CFLAGS) $(LDFLAGS) $(FILES2) -o $@
	@echo "make $@ finished on `date`"

satelliteinfo : $(FILES3)
	$(CC) $(CFLAGS) $(LDFLAGS) $(FILES3) -o $@
	@echo "make $@ finished on `date`"

soap_check_fw : $(FILES4)
	$(CC) $(CFLAGS) $(LDFLAGS) $(FILES4) -o $@
	@echo "make $@ finished on `date`"

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o 
	rm -f soapclient soap_agent satelliteinfo soap_check_fw

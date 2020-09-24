CFLAGS += -std=gnu11 -Wall -Werror -O3

all : asroute

install :
	cp asroute /usr/local/bin/

uninstall : 
	rm -f /usr/local/bin/asroute

asroute : asroute.o

clean : 
	rm -f asroute *.o

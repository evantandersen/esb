# Created Oct 2012
#
# By Evan Andersen 
#
# esb slave


OBJECTS = global.o main.o md5.o network.o slave.o tester.o storage_protocol.o

default:
	$(MAKE) -j $(shell grep -c ^processor /proc/cpuinfo) slave --no-print-directory

slave: $(OBJECTS)
	gcc $(OBJECTS) -lm -lpthread  -g -O2 -std=gnu99 -o esb-slave

%.o : %.c
	gcc -std=gnu99 -c -O2 -g $<

clean:
	rm -rf $(OBJECTS)
	rm -rf esb-slave


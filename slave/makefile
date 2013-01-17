# Created Oct 2012
#
# By Evan Andersen 
#
# esb slave


OBJECTS = global.o main.o md5.o network.o slave.o tester.o

default: $(OBJECTS)
	gcc $(OBJECTS) -lm -lpthread  -g -Os -std=gnu99 -o esb-slave -ljansson libstorage.a

%.o : %.c
	gcc -std=gnu99 -c -O2 -g $<

clean:
	rm -rf $(OBJECTS)
	rm -rf esb-slave


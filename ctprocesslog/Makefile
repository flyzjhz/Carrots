CC = gcc -g
INCS = -I./confparser/ -I/usr/include/ -I/usr/local/include/ 
LDS = -L/usr/local/lib64/ -L/usr/local/lib/ 

OBJS = ctmaster

default: ctmaster

ctmaster: ./confparser/libconfparser.a ctmaster.o ctlog.o ctutils.o  
	$(CC) -o ctmaster ctmaster.o ctlog.o ctutils.o ./confparser/libconfparser.a $(INCS) $(LDS)

ctmaster.o:	ctmaster.c
	$(CC) -c -o ctmaster.o ctmaster.c $(INCS)


ctlog.o:	ctlog.c
	$(CC) -c -o ctlog.o ctlog.c $(INCS)

ctutils.o: ctutils.c
	$(CC) -c -o ctutils.o ctutils.c $(INCS) 


./confparser/libconfparser.a:
	cd ./confparser; make -f linux.mk; cd -



clean:
	cd ./confparser; make -f linux.mk clean; cd -
	rm -rf *.o
	rm -rf $(OBJS)
	

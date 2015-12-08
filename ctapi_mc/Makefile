CC = gcc -g
INCS = -I/usr/include/ -I/usr/local/include/ 
LDS = -L/usr/lib64/ -L/usr/local/lib/ -lmemcached

OBJS = ctapi_mc

default: ctapi_mc

ctapi_mc: ctapi_mc.c ctlog.o 
	$(CC) -DAPIMC_TEST -o ctapi_mc ctapi_mc.c ctlog.o $(INCS) $(LDS)

ctapi_mc.o: ctapi_mc.c
	$(CC) -DAPIMC_TEST -c -o ctapi_mc.o ctapi_mc.c $(INCS)

ctlog.o:	ctlog.c
	$(CC) -c -o ctlog.o ctlog.c $(INCS)




clean:
	rm -rf *.o
	rm -rf $(OBJS)
	

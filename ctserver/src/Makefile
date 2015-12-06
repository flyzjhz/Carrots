CC = gcc -g
INCS = -I../confparser/ -I../libmfile/ -I/usr/include/ -I/usr/local/include/ 
LDS = -L/usr/local/lib64/ -L/usr/local/lib/ -luuid -lcurl 

OBJS = ctserver ctchild

default: ctserver ctchild

ctserver: ../confparser/libconfparser.a ctserver.o ctlog.o ctutils.o  
	$(CC) -o ctserver ctserver.o ctlog.o ctutils.o ../confparser/libconfparser.a $(INCS) $(LDS)

ctserver.o:	ctserver.c
	$(CC) -c -o ctserver.o ctserver.c $(INCS)

ctchild: ../confparser/libconfparser.a ../libmfile/libmfile.a ctchild.o ctlog.o ctutils.o \
	ctlogicprocess.o
	$(CC) -o ctchild ctchild.o ctlog.o ctutils.o ctlogicprocess.o \
		../confparser/libconfparser.a ../libmfile/libmfile.a $(INCS) $(LDS)

ctchild.o: ctchild.c
	$(CC) -c -o ctchild.o ctchild.c $(INCS)



ctlog.o:	ctlog.c
	$(CC) -c -o ctlog.o ctlog.c $(INCS)

ctutils.o: ctutils.c
	$(CC) -c -o ctutils.o ctutils.c $(INCS) 

ctlogicprocess.o: ctlogicprocess.c
	$(CC) -c -o ctlogicprocess.o ctlogicprocess.c $(INCS)


../confparser/libconfparser.a:
	cd ../confparser; make -f linux.mk; cd -

../libmfile/libmfile.a:
	cd ../libmfile; make; cd -




clean:
	cd ../confparser; make -f linux.mk clean; cd -
	cd ../libmfile; make clean; cd -
	rm -rf *.o
	rm -rf $(OBJS)
	

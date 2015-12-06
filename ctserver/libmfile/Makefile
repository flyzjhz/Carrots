CC 	= gcc 
CFLAGS	= -O3 -fPIC

AR = ar
ARFLAGS = -rsv

IFLAGS  = -I/usr/local/include
LDFLAGS = -L/usr/local/lib64

INIDIR = /usr/local/etc/libmfile
INCDIR = /usr/local/include/libmfile
LIBDIR = /usr/local/lib64/libmfile


default: libmfile.a libmfile.so


libmfile.a:	mfile.o
	$(AR) $(ARFLAGS) libmfile.a mfile.o


libmfile.so: mfile.o
	$(CC) -shared -o libmfile.so mfile.o


mfile.o: mfile.c 
	$(CC) $(CFLAGS) -c -o mfile.o mfile.c



install:
	test -d $(INIDIR) || mkdir -p $(INIDIR)	
	test -d $(INCDIR) || mkdir -p $(INCDIR)
	cp -f mfile.h $(INCDIR)/mfile.h
	cp -f queue.h $(INCDIR)/queue.h
	test -d $(LIBDIR) || mkdir -p $(LIBDIR)
	cp -f libmfile.so $(LIBDIR)/libmfile.so
	cp -f libmfile.a $(LIBDIR)/libmfile.a

deinstall:
	rm -rf $(INIDIR)
	rm -rf $(INCDIR)
	rm -rf $(LIBDIR)


test:
	$(CC) -g -o sample sample.c ./libmfile.a -I./


clean:
	rm -f *.o
	rm -f libmfile.a
	rm -f libmfile.so
	rm -f sample



CC 	= gcc
AR = ar
INSTALL	= install
IFLAGS  = -I/usr/local/include
CFLAGS	= $(cflags) -fPIC
PREDEF = 
LDFLAGS = -L/usr/local/lib

INCDIR=/usr/local/include/libctmfile
LIBDIR=/usr/local/lib64/libctmfile

LINK	= -Wl
OBJS	= mfile.o
DIRS	= 
WARN    = $(warn)


.PHONY: clean

.c.o:
	$(CC) $(WARN) -c $*.c $(CFLAGS) $(IFLAGS) $(PREDEF)

libctmfile.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)



install:
	#mv -f $@ ../../lib/
	#cp -f mfile.h /usr/local/include/
	#cp -f queue.h /usr/local/include/
	test -d $(INCDIR) || mkdir $(INCDIR)
	cp -f mfile.h $(INCDIR)/mfile.h
	cp -f queue.h $(INCDIR)/queue.h
	test -d $(LIBDIR) || mkdir $(LIBDIR)
	cp -f libctmfile.a $(LIBDIR)/

clean:
	#rm -f $(OBJS) ../../lib/libctmfile.a
	rm -f *.o
	rm -f libctmfile.a
	rm -f $(OBJS) $(LIBDIR)/libctmfile.a
	rm -f $(INCDIR)/mfile.h
	rm -f $(INCDIR)/queue.h



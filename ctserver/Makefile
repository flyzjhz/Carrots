CC = gcc -g
INCS = -I../confparser/ -I../libmfile/ -I/usr/include/ -I/usr/local/include/ 
LDS = -L/usr/local/lib64/ -L/usr/local/lib/ -luuid -lcurl 

OBJPATH = bin

OBJS = $(OBJPATH)/ctserver $(OBJPATH)/ctchild

default: $(OBJS)

$(OBJS):
	cd src; make; cd -	
	cp -f src/ctserver $(OBJPATH)/ctserver
	cp -f src/ctchild $(OBJPATH)/ctchild



clean:
	cd src; make clean; cd -
	rm -rf $(OBJS)
	

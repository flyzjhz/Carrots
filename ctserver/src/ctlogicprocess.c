//
//  ctlogicprocess.c
//  
//
//  Created by SongJian on 15/12/5.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/param.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <strings.h>

#include "ctlog.h"
#include "ctutils.h"

#include "mfile.h"

#include "ctlogicprocess.h"


#define MAX_LINE    1024



void socket_protocol_process(MFILE *mfp, int sockfd)
{
    // read
    char buf[MAX_LINE] = {0};
    char ch;
    int i, n;
    
    for (i=0; i<=msize(mfp) ;i++) {
        ch = mgetc(mfp);
		log_debug("get ch:%c", ch);
        if (ch == 0 || ch == ' ') {
            break;
        }
        
        buf[i] = ch;
    }
    
    if (strcasecmp(buf, "QUIT\r\n") == 0) {
		char tmp_buf[4196] = {0};
		int tmp_buf_len = mread(mfp, tmp_buf, sizeof(tmp_buf));
		log_debug("read data from client:[%d]%s", tmp_buf_len, tmp_buf);

        n = snprintf(buf, sizeof(buf), "220 Bye.\r\n");
        n = write(sockfd, buf, n);
        
        _exit(110);
    }

	mseek_pos(mfp, msize(mfp));

	n = snprintf(buf, sizeof(buf), "250 OK\r\n");
	n = write(sockfd, buf, n);
}

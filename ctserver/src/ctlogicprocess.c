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


extern struct config_st config_t;
extern int pfd_r;
extern int pfd_w;
extern int client_fd;


// 用于与客户端读写buffer
extern MFILE *mfp_in;

extern char sid[MAX_LINE];
extern char client_ip[MAX_LINE];
extern char client_port[MAX_LINE];




void socket_protocol_process(MFILE *mfp, int sockfd)
{
	mseek(mfp);

    // read
    char cmd_type[MAX_LINE] = {0};
    char cmd_name[MAX_LINE] = {0};
    int flag = 0;
    int read_line_finish = 1;
    char buf[MAX_LINE] = {0};
    char *pbuf = buf;
    char ch;
    int i, n, j;

    char response_buf[MAX_LINE] = {0};

	while (1) {
        // 从mfp中逐行取每条指令处理
        memset(buf, 0, sizeof(buf));
        n = mread_line(mfp, buf, sizeof(buf));
        if (n == 0) {
            if (*buf == '\0') {
                break;
            }

            // 未读完一行，mfp中还有数据，这行太长了
            read_line_finish = 0;
        }
        log_debug("read data from mfp data:[%d]%s", n, buf);

        flag = 0;
        for (i=0,j=0; i<=n ;i++) {
            ch = buf[i];
    		//log_debug("%s get ch:%c", sid, ch);

            if (ch == 0 || ch == '\r' || ch == '\n') {
                break;
            }

            if (ch == ' ' || ch == '\r' || ch == '\n') {
                if (flag == 0) {
                    cmd_type[j] = '\0';
                } else {
                    cmd_name[j] = '\0';

                    break;
                }

                flag++;
                j = 0;
                continue;
            }

            if (flag == 0) {
                cmd_type[j] = ch;
            } else if (flag == 1) {
                cmd_name[j] = ch;
            }

            j++;
        }

        if (strcasecmp(buf, "QUIT\r\n") == 0) {
    		char tmp_buf[4196] = {0};
    		int tmp_buf_len = mread(mfp, tmp_buf, sizeof(tmp_buf));
    		log_debug("%s read data from client:[%d]%s", sid, tmp_buf_len, tmp_buf);

            n = snprintf(buf, sizeof(buf), "220 Bye.\r\n");
            n = write(sockfd, buf, n);

            _exit(110);

        } else {
			int nw = write(client_fd, pbuf+i-strlen(cmd_name), strlen(pbuf+i)+strlen(cmd_name));
			if (nw < 0) {
				n = snprintf(buf, sizeof(buf), "FAIL To Write");
                write(sockfd, buf, n);
                return ;
			}
			log_debug("%s write data to fd:%d data:[%d]%s", sid, pfd_w, nw, pbuf+i-strlen(cmd_name));

			if (read_line_finish == 0) {
            	while (((n = mread_line(mfp, buf, sizeof(buf))) == 0) && (*buf != '\0')) {
					nw = write(client_fd, buf, n);
					if (nw < 0) {
						log_error("%s write to fd:%d fail:%m", sid, client_fd);
						return;
					}
					log_debug("%s write to fd:%d data:[%d]%s", sid, pfd_w, nw, pbuf);
					memset(buf, 0, sizeof(buf));
				}	
				if (*buf != '\0') {
					nw = write(client_fd, buf, n);
					if (nw < 0) {
						log_error("%s write to fd:%d fail:%m", sid, client_fd);
						return;
					}
					log_debug("%s write to client fd:%d data:[%d]%s", sid, client_fd, nw, buf);
				}
			}

		}

    }
}

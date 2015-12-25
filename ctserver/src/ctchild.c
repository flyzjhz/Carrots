//
//  ctchild.c
//  
//
//  Created by SongJian on 15/12/4.
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

#include "confparser.h"
#include "dictionary.h"
#include "mfile.h"

#include "ctchild.h"
#include "ctlog.h"
#include "ctutils.h"

#include "ctlogicprocess.h"


#define FAIL_CODE       "5.1.0"
#define END_DATA        (u_char *) "\r\n"


extern void socket_protocol_process(MFILE *mfp, int sockfd);
extern void set_check_client_timeout(unsigned int timeout);


char sid[MAX_LINE] = {0};
char client_ip[MAX_LINE] = {0};
char client_port[MAX_LINE] = {0};


// fd 说明:
//  0:  从父进程读
//  1:  向父进程写
//  2:  客户端socket fd
int pfd_r = 0;
int pfd_w = 1;
int client_fd = 2;

// 用于与客户端读写buffer
MFILE *mfp_in;

int buf_size;
int max_blocksize;


// save config from read config file
dictionary *dict_conf          = NULL;
struct config_st config_t;

// epoll
int epoll_fd                   = -1;
int epoll_nfds                 = -1;
int epoll_event_num            = 0;
struct epoll_event *epoll_evts = NULL;



dictionary *open_config(char *config_file)
{
    dictionary *dict = open_conf_file(config_file);
    if (dict == NULL) {
        log_error("open config file fail:%s", config_file);
        return NULL;
    }
    
    return dict;
}

void close_config(dictionary *dict)
{
    if (dict != NULL) {
        dictionary_del(dict);
        dict = NULL;
    }
}

int read_config(dictionary *dict)
{
    if (dict == NULL) {
        printf("dictionary is NULL, please call open_config first\n");
        return 1;
    }
    
    // log level
    char *plog_level = dictionary_get(dict, "global:log_level", NULL);
    if (plog_level == NULL) {
        log_warning("parse config 'log_level' fail, use default:%s", DEF_LOGLEVEL);
        snprintf(config_t.log_level, sizeof(config_t.log_level), "%s", DEF_LOGLEVEL);
    } else {
        snprintf(config_t.log_level, sizeof(config_t.log_level), "%s", plog_level);
    }
    
    // network fd r/w timeout
    char *pconntimeout = dictionary_get(dict, "global:conntimeout", NULL);
    if (pconntimeout == NULL) {
        log_warning("parse config 'conntimeout' fail, use default:%s", DEF_CONNTIMEOUT);
        snprintf(config_t.conntimeout, sizeof(config_t.conntimeout), "%s", DEF_CONNTIMEOUT);
    } else {
        snprintf(config_t.conntimeout, sizeof(config_t.conntimeout), "%s", pconntimeout);
    }
    
    // network connect timout
    char *prwtimeout = dictionary_get(dict, "global:rwtimeout", NULL);
    if (prwtimeout == NULL) {
        log_warning("parse config 'rwtimeout' fail, use default:%s", DEF_RWTIMEOUT);
        snprintf(config_t.rwtimeout, sizeof(config_t.rwtimeout), "%s", DEF_RWTIMEOUT);
    } else {
        snprintf(config_t.rwtimeout, sizeof(config_t.rwtimeout), "%s", prwtimeout);
    }
    
    // buffer size for mfile buffer
    char *pbuf_size = dictionary_get(dict, "global:buf_size", NULL);
    if (pbuf_size == NULL) {
        log_warning("parse config 'buf_size' fail, use default:%s", BUF_SIZE);
        snprintf(config_t.buf_size, sizeof(config_t.buf_size), "%s", BUF_SIZE);
    } else {
        snprintf(config_t.buf_size, sizeof(config_t.buf_size), "%s", pbuf_size);
    }
    
    // block size for mfile buffer
    char *pmax_blocksize = dictionary_get(dict, "global:max_blocksize", NULL);
    if (pmax_blocksize == NULL) {
        log_warning("parse config 'max_blocksize' fail, use default:%s", MAX_BLOCK_SIZE);
        snprintf(config_t.max_blocksize, sizeof(config_t.max_blocksize), "%s", MAX_BLOCK_SIZE);
    } else {
        snprintf(config_t.max_blocksize, sizeof(config_t.max_blocksize), "%s", pmax_blocksize);
    }
    
    return 0;
}


/**
 *  write buffer to MFILE
 *
 *  @param mfp     需要被写进的mfp
 *  @param buf     要被写入的buffer
 *  @param buf_len buffer的长度
 *
 *  @return 0: 失败   1: 成功
 */
int fast_write(MFILE *mfp, char *buf, size_t buf_len)
{
    int nw = mwrite(mfp, buf, buf_len);
    if (nw != 1) {
        //mclose(mfp);
        //mfp = NULL;
        return 0;
    }
    
    return nw;
}




/**
 *  读取fd内容并保存到mfile中
 *
 *  @param sockfd      目标读取的fd
 *  @param end_tag     逻辑段结束标记字符串, 表示每个读取结束标记, 比如\r\n
 *  @param end_tag_len 结束标记字符串的长度
 *
 *  @return 保存的字节数, <0 出错
 */
int read_buf_from_sockfd(int sockfd, char *end_tag, size_t end_tag_len)
{
    char buf[buf_size];
    char tbuf[buf_size];
    char *ptbuf = tbuf;
    int tbuf_size = sizeof(tbuf);
    int tbuf_len = 0;
    int n = 0;
    int read_total_bytes = 0;
    int state = 0;
    char ch;
    
    memset(buf, 0, sizeof(buf));
    memset(tbuf, 0, sizeof(tbuf));
    tbuf_len = 0;
    ptbuf = tbuf + tbuf_len;
    
    int i = 0;
    n = read(sockfd, buf, sizeof(buf));
    if (n <= 0) {
        return n;
    }
    
    while (i < n) {
        ch = buf[i];
        if ((tbuf_size - tbuf_len) < 2) {
            // 如果临时内存tbuf满了，一次性刷进mfile中
            int nw = fast_write(mfp_in, tbuf, tbuf_len);
            if (nw == 0) {
                log_error("fast_write fail");
                
                //mclose(mfp_in);
                //mfp_in = NULL;
                
                return -1;
            }
            read_total_bytes += tbuf_len;
            
            memset(tbuf, 0, tbuf_size);
            tbuf_len = 0;
        }
        
        // 追加1个字节ch到临时内存tbuf
        byte_copy(ptbuf + tbuf_len, 1, &ch);
        tbuf_len++;
        
        if (ch == end_tag[state]) {
            state++;
            
            if (state == end_tag_len) {
                // ------ 完成一次逻辑段读取 ------
                // 1. 把剩余的tbuf刷进mfile
                if (tbuf_len > 0) {
                    int nw = fast_write(mfp_in, tbuf, tbuf_len);
                    if (nw == 0) {
                        log_error("fast_write fail");
                        
                        //mclose(mfp_in);
                        //mfp_in = NULL;
                        
                        return -1;
                    }
                    read_total_bytes += tbuf_len;
                    
                    memset(tbuf, 0, tbuf_size);
                    tbuf_len = 0;
                }
                
                // 2. 返回
                return read_total_bytes;
            }
            
        } else {
            state = 0;
        }
        
        i++;
    }
}



/**
 *  读取fd内容并保存到mfile中
 *
 *  @param sockfd      目标读取的fd
 *  @param mfp		   保存到mfp中
 *
 *  @return -1:出错 0:进程退出 >0:保存的字节数
 */
int read_buf_to_mfp_from_sockfd(int sockfd, MFILE *mfp)
{
	char buf[BUF_SIZE] = {0};
    int length = 0;
    int nr = 0;
    int nw = 0;
    for (;;) {
        nr = read(sockfd, buf, sizeof(buf));
        if (nr == -1) {
            // If errno == EAGAIN, that means we have read all data.
			// So go back to the main loop
            if (errno != EAGAIN) {
                // 读取出错
                log_error("%s read socket fd:%d fail:%m", sid, sockfd);

                return -1;
            }

            return length;

        } else if (nr == 0) {
            // End of file. The remote has closed the connection.
            return 0;
        }
        log_debug("%s read data from socket fd:%d data:[%d]%s", sid, sockfd, nr, buf);

		nw = mwrite(mfp, buf, nr);
        if (nw != 1) {
            log_error("%s mwrite fail for buffer:[%d]%s", sid, nr, buf);
            return -1;
        }

        length += nw;
    }

    return length;
}







void usage(char *prog)
{
    printf("%s -m[unique id] -c[child config file] -r[client ip:port]\n", prog);
}


// fd 说明:
//  0:  从父进程读
//  1:  向父进程写
//  2:  客户端socket fd
int main(int argc, char **argv)
{
    // init log
    ctlog(argv[0], LOG_PID|LOG_NDELAY, LOG_MAIL);
    
    char cfg_file[MAX_LINE] = {0};
    char sid[MAX_LINE] = {0};
    char client_ip[MAX_LINE] = {0};
    char client_port[MAX_LINE] = {0};
    
    int ch;
    const char *args = "m:c:r:h";
    while ((ch = getopt(argc, argv, args)) != -1) {
        switch (ch) {
            case 'm':
                snprintf(sid, sizeof(sid), "%s", optarg);
                break;
            case 'c':
                snprintf(cfg_file, sizeof(cfg_file), "%s", optarg);
                break;
            case 'r':
                snprintf(client_ip, sizeof(client_ip), "%s", optarg);
                char *pport = (char *)memchr(client_ip, ':', strlen(client_ip));
                if (pport != NULL) {
                    *pport = '\0';
                    snprintf(client_port, sizeof(client_port), "%s", pport+1);
                }
                break;
                
            case 'h':
            default:
                usage(argv[0]);
                exit(0);
                break;
        }
    }
    
    // read config
    dict_conf = open_config(cfg_file);
    if (dict_conf == NULL) {
        printf("parse config fail");
        return 1;
    }
    if (read_config(dict_conf) != 0) {
        return 1;
    }
    log_level = atoi(config_t.log_level);
    
    log_debug("log_level:%s", config_t.log_level);
    log_debug("conntimeout:%s", config_t.conntimeout);
    log_debug("rwtimeout:%s", config_t.rwtimeout);
    log_debug("buf_size:%s", config_t.buf_size);
    log_debug("max_blocksize:%s", config_t.max_blocksize);
    
    
    // ------ Init Memory Buffer For Read/Write From Client ------
    buf_size = atoi(config_t.buf_size);
    max_blocksize = atoi(config_t.max_blocksize);
    
	mfp_in = NULL;
    /*mfp_in = mopen(max_blocksize, NULL, NULL);
    if (mfp_in == NULL) {
        log_error("mopen fail");
        return 1;
    }*/
    
    
    // Get Local Host Name
    char local_hostname[MAX_LINE] = {0};
    if (gethostname(local_hostname, sizeof(local_hostname)) != 0) {
        snprintf(local_hostname, sizeof(local_hostname), "unknown");
    }
    log_debug("local_hostname:%s", local_hostname);
    
    
    // ------ Send Greeting To Client -----
    char greeting_buf[MAX_LINE] = {0};
    int n = snprintf(greeting_buf, sizeof(greeting_buf), "Greeting %s %s %s", local_hostname, sid, END_DATA);
    n = write(client_fd, greeting_buf, n);
    log_info("%s send to client fd[%d]:[%d]%s", sid, client_fd, n, greeting_buf);
    
    
    // epoll initialize
    int epoll_i = 0;
    epoll_event_num = 4;
    epoll_evts = (struct epoll_event *)malloc(epoll_event_num * sizeof(struct epoll_event));
    if (epoll_evts == NULL) {
        log_error("alloc epoll event failed");
        goto CTEXIT;
    }
    
    // epoll create fd
    epoll_fd = epoll_create(epoll_event_num);
    if (epoll_fd <= 0) {
        log_error("create epoll fd failed:[%d]%s", errno, strerror(errno));
        goto CTEXIT;
    }
    
    
    // add fd 0 to epoll for send -------
    if (set_nonblocking(client_fd) != 0) {
        log_error("setnonblocking fd[%d] failed", client_fd);
    }
    
    struct epoll_event client_evt;
    client_evt.events = EPOLLIN | EPOLLET;
    client_evt.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_evt) == -1) {
        log_info("add event to epoll fail. fd:%d event:EPOLLIN", client_evt.data.fd);
        goto CTEXIT;
    }
    log_debug("add fd:%d event to epoll succ", client_evt.data.fd);
    
    
    // add parent read fd pipe_r to epoll for send -------
    if (set_nonblocking(pfd_r) != 0) {
        log_error("setnonblocking parent pipe read fd[%d] failed", pfd_r);
    }
    
    struct epoll_event pfd_r_evt;
    pfd_r_evt.events = EPOLLIN | EPOLLET;
    pfd_r_evt.data.fd = pfd_r;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pfd_r, &pfd_r_evt) == -1) {
        log_info("add event to epoll fail. fd:%d event:EPOLLIN",  pfd_r_evt.data.fd);
        goto CTEXIT;
    }
    log_debug("add parent pipe read fd:%d event to epoll succ", pfd_r_evt.data.fd);
    
    int rw_timeout = (atoi(config_t.rwtimeout)) * 1000;
    
    for (;;) {
        epoll_nfds = epoll_wait(epoll_fd, epoll_evts, epoll_event_num, rw_timeout);
        if (epoll_nfds == -1) {
            if (errno == EINTR) {
                log_info("epoll_wait recive EINTR signal, continue");
                continue;
            }
            
            goto CTEXIT;
        } else if (epoll_nfds == 0) {
            log_info("epoll_wait client timeout[%d s], exit", atoi(config_t.rwtimeout));
            
            goto CTEXIT;
        }
        
        
        for (epoll_i = 0; epoll_i < epoll_nfds; epoll_i++) {
            int evt_fd = epoll_evts[epoll_i].data.fd;
            int evt_event = epoll_evts[epoll_i].events;
            
            log_debug("epoll[%d] fd:%d event:%d", epoll_i, evt_fd, epoll_evts[epoll_i].events);
            if ((evt_event & EPOLLIN) && (evt_fd == pfd_r)) {
                // get read event from Parent
                log_debug("get event EPOLLIN from Parent socket fd:%d", evt_fd);
                
                // ...
            } else if ((evt_event & EPOLLIN) && (evt_fd == client_fd)) {
                // get read event from Client
                log_debug("get event EPOLLIN from Client socket fd:%d", evt_fd);

				mfp_in = mopen(max_blocksize, NULL, NULL);
    			if (mfp_in == NULL) {
        			log_error("mopen fail");
        			return 1;
    			}  

				int nr = read_buf_to_mfp_from_sockfd(evt_fd, mfp_in);
				if (nr == -1 || nr == 0) {
                    // 1. 出错，可能管道出错，需要退出重新建立
                    // 2. 进程退出了
                    close(evt_fd);
                    close(pfd_r);
                    close(pfd_w);

                    goto CTEXIT;

                } else {
                    // process logic
                    socket_protocol_process(mfp_in, evt_fd);

                }
                
                // ...
                
            } else if (evt_event & EPOLLOUT) {
                log_debug("get event EPOLLOUT socket fd:%d", evt_fd);
                
            } else if (evt_event & EPOLLHUP) {
                log_debug("get event EPOLLHUP socket fd:%d", evt_fd);
                
            }
            
        }
    }
    

CTEXIT:
    if (mfp_in) {
        mclose(mfp_in);
        mfp_in = NULL;
    }
	if (epoll_evts) {
		free(epoll_evts);
		epoll_evts = NULL;
	}

    _exit(111);
    

}

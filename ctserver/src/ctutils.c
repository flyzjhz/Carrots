//
//  ctutils.c
//  
//
//  Created by SongJian on 15/12/3.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/epoll.h>
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
#include <uuid/uuid.h>

#include "ctlog.h"
#include "ctutils.h"



/**
 *  设置指定的描述符为非阻塞
 *
 *  @param fd 指定的描述符
 *
 *  @return 0:成功 其它:失败
 */
int set_nonblocking(int fd)
{
    int opts = fcntl(fd, F_GETFL);
    if (opts < 0) {
        log_error("fcntl F_GETFL failed:[%d]:%s", errno, strerror(errno));
        return 1;
    }
    
    opts = (opts | O_NONBLOCK);
    if (fcntl(fd, F_SETFL, opts) < 0) {
        log_error("fcntl F_SETFL failed:[%d]:%s", errno, strerror(errno));
        return 1;
    }
    
    return 0;
}


/**
 *  使用UUID，生成唯一ID
 *
 *  @param mid      返回的id buffer
 *  @param mid_size id buffer 大小
 *
 *  @return 16:成功  其它:失败
 */
int create_unique_id(char *mid, size_t mid_size)
{
    uuid_t uuid;
    uuid_generate(uuid);
    
    unsigned char *p = uuid;
    int i;
    char ch[5] = {0};
    for (i=0; i<sizeof(uuid_t); i++,p++) {
        snprintf(ch, sizeof(ch), "%02X", *p);
        mid[i*2] = ch[0];
        mid[i*2+1] = ch[1];
    }
    
    return i;
}


int fd_copy(int to, int from)
{
    if (to == from)
        return 0;
    if (fcntl(from, F_GETFL, 0) == -1)
        return -1;
    close(to);
    if (fcntl(from, F_DUPFD, to) == -1)
        return -1;
    return 0;
}

int fd_move(int to, int from)
{
    if (to == from)
        return 0;
    if (fd_copy(to, from) == -1)
        return -1;
    close(from);
    return 0;
}


void sig_catch( int sig, void (*f) () )
{
    struct sigaction sa;
    sa.sa_handler = f;
    sa.sa_flags   = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(sig, &sa, (struct sigaction *) 0);
}

void sig_block(int sig)
{
    sigset_t ss;
    sigemptyset(&ss);
    sigaddset(&ss, sig);
    sigprocmask(SIG_BLOCK, &ss, (sigset_t *) 0);
}

void sig_unblock(int sig)
{
    sigset_t ss;
    sigemptyset(&ss);
    sigaddset(&ss, sig);
    sigprocmask(SIG_UNBLOCK, &ss, (sigset_t *) 0);
}

void sig_pipeignore()
{
    sig_catch(SIGPIPE, SIG_IGN);
}

void sig_childblock()
{
    sig_block(SIGCHLD);
}

void sig_childunblock()
{
    sig_unblock(SIGCHLD);
}



/**
 *	从from拷贝n字节到to
 * 
 *	@param to   目标地址
 * 	@param n    拷贝的字节数
 *	@param from 源地址
 */
void byte_copy(register char *to, register unsigned int n, register char *from)
{
    for (;;) {
        if (!n) {
            return;
        }   
        *to++ = *from++;
        --n;
    
        if (!n) {
            return;
        }   
        *to++ = *from++;
        --n;
    
        if (!n) {
            return;
        }   
        *to++ = *from++;
        --n;
    }   
}



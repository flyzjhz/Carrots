//
//  ctserver.c
//  
//
//  Created by SongJian on 15/12/2.
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
#include "ctlog.h"
#include "ctutils.h"

#include "ctserver.h"


#define FAIL_CODE       "5.0.0"

#define DATA_END		(u_char *) "\r\n"


// save config from read config file
dictionary *dict_conf          = NULL;

struct config_st config_t;
struct childs_st *childs_t     = NULL;


// epoll
int epoll_fd                   = -1;
int epoll_nfds                 = -1;
int epoll_event_num            = 0;
struct epoll_event *epoll_evts = NULL;
int epoll_num_running          = 0;

int listen_fd                  = -1;



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
    
    // bind port
    char *pbind_port = dictionary_get(dict, "global:bind_port", NULL);
    if (pbind_port == NULL) {
        log_warning("parse config 'bind_port' fail, use default:%s", DEF_BINDPORT);
        snprintf(config_t.bind_port, sizeof(config_t.bind_port), "%s", DEF_BINDPORT);
    } else {
        snprintf(config_t.bind_port, sizeof(config_t.bind_port), "%s", pbind_port);
    }
    
    // log level
    char *plog_level = dictionary_get(dict, "global:log_level", NULL);
    if (plog_level == NULL) {
        log_warning("parse config 'log_level' fail, use default:%s", DEF_LOGLEVEL);
        snprintf(config_t.log_level, sizeof(config_t.log_level), "%s", DEF_LOGLEVEL);
    } else {
        snprintf(config_t.log_level, sizeof(config_t.log_level), "%s", plog_level);
    }
    
    // change dir
    char *pchdir_path = dictionary_get(dict, "global:chdir_path", NULL);
    if (pchdir_path == NULL) {
        log_warning("parse config 'chdir_path' fail, use default:%s", DEF_CHDIRPATH);
        snprintf(config_t.chdir_path, sizeof(config_t.chdir_path), "%s", DEF_CHDIRPATH);
    } else {
        snprintf(config_t.chdir_path, sizeof(config_t.chdir_path), "%s", pchdir_path);
    }
    
    // max child number
    char *pmax_childs = dictionary_get(dict, "global:max_childs", NULL);
    if (pmax_childs == NULL) {
        log_warning("parse config 'max_childs' fail, use default:%s", DEF_MAXCHILDS);
        snprintf(config_t.max_childs, sizeof(config_t.max_childs), "%s", DEF_MAXCHILDS);
    } else {
        snprintf(config_t.max_childs, sizeof(config_t.max_childs), "%s", pmax_childs);
    }
    
    // child program
    char *pchild_prog = dictionary_get(dict, "global:child_prog", NULL);
    if (pchild_prog == NULL) {
        log_warning("parse config 'child_prog' fail, use default:%s", DEF_CHILDPROG);
        snprintf(config_t.child_prog, sizeof(config_t.child_prog), "%s", DEF_CHILDPROG);
    } else {
        snprintf(config_t.child_prog, sizeof(config_t.child_prog), "%s", pchild_prog);
    }
    
    // child config file
    char *pchild_cf = dictionary_get(dict, "global:child_cf", NULL);
    if (pchild_cf == NULL) {
        log_warning("parse config 'child_cf' fail, use default:%s", DEF_CHILDINI);
        snprintf(config_t.child_cf, sizeof(config_t.child_cf), "%s", DEF_CHILDINI);
    } else {
        snprintf(config_t.child_cf, sizeof(config_t.child_cf), "%s", pchild_cf);
    }
    
    
    return 0;
}




/**
 *  获取一个空闲的索引
 *
 *  @return 返回索引，－1: 失败
 */
int get_idle_idx_from_childs_t()
{
    int idx = -1;
    int i = 0;
    for (i = 0; i<(atoi(config_t.max_childs) + 1); i++) {
        if (childs_t[i].used == 0) {
            idx = i;
            break;
        }
    }
    
    return idx;
}

int get_idx_with_sockfd(int sockfd)
{
    int idx = -1;
    int i = 0;
    for (i=0; i<(atoi(config_t.max_childs) + 1); i++) {
        if ((childs_t[i].pfd_r == sockfd)
            && (childs_t[i].used == 1)) {
            idx = i;
            break;
        }
    }
    
    return i;
}


void init_child_with_idx(int i)
{
    childs_t[i].used = 0;
    childs_t[i].pid = -1;
    childs_t[i].pfd_r = -1;
    childs_t[i].pfd_w = -1;
    memset(childs_t[i].sid, 0, sizeof(childs_t[i].sid));
    
    childs_t[i].client_info.fd = -1;
    memset(childs_t[i].client_info.ip, 0, sizeof(childs_t[i].client_info.ip));
    memset(childs_t[i].client_info.port, 0, sizeof(childs_t[i].client_info.port));
}

void clean_child_with_idx(int i)
{
    log_debug("clean child index:%d", i);
    
    // clean child
    childs_t[i].used = 0;
    childs_t[i].pid = -1;
    memset(childs_t[i].sid, 0, sizeof(childs_t[i].sid));
    if (childs_t[i].pfd_r != -1) {
        close(childs_t[i].pfd_r);
        childs_t[i].pfd_r = -1;
    }
    if (childs_t[i].pfd_w != -1) {
        close(childs_t[i].pfd_w);
        childs_t[i].pfd_w = -1;
    }
    
    // clean client info
    if (childs_t[i].client_info.fd != -1) {
        close(childs_t[i].client_info.fd);
        childs_t[i].client_info.fd = -1;
    }
    memset(childs_t[i].client_info.ip, 0, sizeof(childs_t[i].client_info.ip));
    memset(childs_t[i].client_info.port, 0, sizeof(childs_t[i].client_info.port));
    
}


// return: 0:succ 1:fail
int epoll_event_mod(int sockfd, int type)
{
    struct epoll_event ev;
    ev.data.fd = sockfd;
    ev.events = type;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sockfd, &ev) == -1) {
        log_error("epoll_ctl fd[%d] EPOLL_CTL_MOD EPOLLIN failed:[%d]%s", sockfd, errno, strerror(errno));
        return 1;
    }
    return 0;
}

// return: 0:succ 1:fail
int epoll_delete_evt(int epoll_fd, int fd)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0) == -1) {
        log_error("delete event from epoll fail:%d %s fd:%d", errno, strerror(errno), fd);
        return 1;
    }
    log_debug("delete event from epoll succ. fd:%d", fd);
    
    epoll_num_running--;
    
    return 0;
}


void sigchld_exit()
{
    int wstat, pid, i;
    while ((pid = waitpid(-1, &wstat, WNOHANG)) > 0) {
        /*for (i=0; i<(atoi(config_t.max_childs) + 1); i++) {
            if ((childs_t[i].used == 1) && (childs_t[i].pid == pid)) {
                // catch child exit
                clean_child_with_idx(i);
                
                return;
            }
        }*/
    }
}


void create_daemon(char *chdir_path)
{
    // 1. 后台运行
    pid_t pid1 = fork();
    if (pid1 == -1) {
        printf("fork fail\n");
        exit(1);
    } else if (pid1 > 0) {
        // parent exit
        exit(0);
    }
    
    // 2. 独立于控制终端
    if (setsid() == -1) {
        printf("setsid fail\n");
        exit(1);
    }
    
    // 3. 防止子进程(组长)获取控制终端
    pid_t pid2 = fork();
    if (pid2 == -1) {
        printf("fork fail\n");
        exit(1);
    } else if (pid2 > 0) {
        // parent exit
        exit(0);
    }
    
    // 4. 关闭打开的文件描述符
    int i;
    for (i=0; i<NOFILE; i++) {
        close(i);
    }
    
    // 5. 改变工作目录
    if (chdir_path != NULL) {
        chdir(chdir_path);
    }
    
    // 6. 清除文件创建掩码(umask)
    umask(0);
    
    // 7. 处理信号
    signal(SIGCHLD, SIG_IGN);
    
    return;
}


void usage(char *prog)
{
    printf("%s -c [config file]\n", prog);
}

int main(int argc, char **argv)
{
    if (argc != 3 && argc != 4) {
        usage(argv[0]);
        exit(0);
    }
    
    char cfg_file[MAX_LINE] = {0};
    int make_deamon = 0;
    
    int ch;
    const char *args = "c:dh";
    while ((ch = getopt(argc, argv, args)) != -1) {
        switch (ch) {
            case 'c':
                snprintf(cfg_file, sizeof(cfg_file), "%s", optarg);
                break;
            case 'd':
                make_deamon = 1;
                break;
                
            case 'h':
            default:
                usage(argv[0]);
                exit(0);
                break;
        }
    }
    
    if (make_deamon == 1) {
        // create deamon
        create_daemon(config_t.chdir_path);
    }
    
    // init log
    ctlog("ctserver", LOG_PID|LOG_NDELAY, LOG_MAIL);
    
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
    
    if (chdir(config_t.chdir_path) == -1) {
        log_error("can not start: unable to change directory:%s", config_t.chdir_path);
        return 1;
    }
    log_debug("bind_port:%s", config_t.bind_port);
    log_debug("log_level:%s", config_t.log_level);
    log_debug("chdir_path:%s", config_t.chdir_path);
    log_debug("max_childs:%s", config_t.max_childs);
    log_debug("child_prog:%s", config_t.child_prog);
    log_debug("child_cf:%s", config_t.child_cf);
    
    // Get Local Host Name
    char local_hostname[MAX_LINE] = {0};
    if (gethostname(local_hostname, sizeof(local_hostname)) != 0) {
        snprintf(local_hostname, sizeof(local_hostname), "unknown");
    }
    log_debug("local_hostname:%s", local_hostname);
    
    
    // ---------- ----------
    
    childs_t = (struct childs_st *)malloc((atoi(config_t.max_childs) + 1) * sizeof(struct childs_st));
    if (childs_t == NULL) {
        log_error("malloc childs [%d] faild:[%d]:%s", (atoi(config_t.max_childs) + 1), errno, strerror(errno));
        exit(1);
    }
    
    int i = 0;
    for (i=0; i<(atoi(config_t.max_childs) +1); i++) {
        init_child_with_idx(i);
    }
    
    
    // Start Server
    int connfd, epfd, sockfd, n, nread, nwrite;
    struct sockaddr_in local, remote;
    socklen_t addrlen;
    
    /*char buf[BUF_SIZE] = {0};
    
    size_t pbuf_len = 0;
    size_t pbuf_size = sizeof(buf) + 1;
    char *pbuf = (char *)calloc(1, pbuf_size);
    if (pbuf == NULL) {
        log_error("calloc fail: size[%d]", pbuf_size);
        exit(1);
    }*/
    
    // Create Listen Socket
    int bind_port = atoi(config_t.bind_port);
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_error("socket fail:[%d]:%s", errno, strerror(errno));
        exit(1);
    }
    
    // Set Listen FD Nonblock
    if (set_nonblocking(listen_fd) != 0) {
        exit(1);
    }
    
    bzero(&local, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(bind_port);
    if (bind(listen_fd, (struct sockaddr *)&local, sizeof(local)) < 0) {
        log_error("bind local %d failed:[%d]%s", bind_port, errno, strerror(errno));
        exit(1);
    }
    log_info("bind local %d succ", bind_port);
    
    if (listen(listen_fd, atoi(config_t.max_childs)) != 0) {
        log_error("listen fd[%d] max_number[%d] failed:[%d]%s", listen_fd, atoi(config_t.max_childs), errno, strerror(errno));
        exit(1);
    }
    
    // Ignore pipe signal
    sig_pipeignore();
    // Catch signal which is child program exit
    sig_catch(SIGCHLD, sigchld_exit);
    
    // epoll create fd
    epoll_event_num = atoi(config_t.max_childs) + 1;
    epoll_evts = NULL;
    epoll_fd = -1;
    epoll_nfds = -1;
    
    int epoll_i = 0;
    
    epoll_evts = (struct epoll_event *)malloc(epoll_event_num * sizeof(struct epoll_event));
    if (epoll_evts == NULL) {
        log_error("malloc for epoll event fail");
        exit(1);
    }
    
    epoll_fd = epoll_create(epoll_event_num);
    if (epoll_fd == -1) {
        log_error("epoll_create max_number[%d] failed:[%d]%s", epoll_event_num, errno, strerror(errno));
        exit(1);
    }
    
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
        log_error("epoll_ctl: listen_socket failed:[%d]%s", errno, strerror(errno));
        exit(1);
    }
    
    epoll_num_running = 0;
    
    for (;;) {
        
        epoll_nfds = epoll_wait(epoll_fd, epoll_evts, epoll_event_num, -1);
        
        if (epoll_nfds == -1) {
            if (errno == EINTR) {
                // 收到中断信号
                log_info("epoll_wait recive EINTR signal, continue");
                continue;
            }
            
            exit(1);
        }
        
        log_debug("epoll_num_running:%d nfds:%d", epoll_num_running, epoll_nfds);
        for (epoll_i = 0; epoll_i < epoll_nfds; epoll_i++) {
            sig_childblock();
            
            int evt_fd = epoll_evts[epoll_i].data.fd;
            if (evt_fd == listen_fd) {
                // new connect
                if ((connfd = accept(listen_fd, (struct sockaddr *)&remote, &addrlen)) > 0) {
                    char *ipaddr = inet_ntoa(remote.sin_addr);
                    log_debug("accept client:%s", ipaddr);
                    
                    char greet_buf[MAX_LINE] = {0};
                    
                    // get a new index from child list
                    int i = get_idle_idx_from_childs_t();
                    if (i == -1) {
                        log_error("get_idle_idx_from_childs_t fail: maybe client queue is full.");
                        
                        // send to client error information
                        n = snprintf(greet_buf, sizeof(greet_buf), "%s ERR %s%s", FAIL_CODE, local_hostname, DATA_END);
                        nwrite = write(connfd, greet_buf, n);
                        log_debug("send client fd[%d]:[%d]%s", connfd, nwrite, greet_buf);
                        
                        continue;
                    }
                    childs_t[i].used = 1;
                    
                    // get client ip and port.
                    struct sockaddr_in sa;
                    int len = sizeof(sa);
                    if (!getpeername(connfd, (struct sockaddr *)&sa, &len)) {
                        n = snprintf(childs_t[i].client_info.ip, sizeof(childs_t[i].client_info.ip), "%s", inet_ntoa(sa.sin_addr));
                        n = snprintf(childs_t[i].client_info.port, sizeof(childs_t[i].client_info.port), "%d", ntohs(sa.sin_port));
                        log_info("accept client:%s:%s", childs_t[i].client_info.ip, childs_t[i].client_info.port);
                    }
                    
                    
                    int pi1[2];
                    int pi2[2];
                    if (pipe(pi1) == -1) {
                        log_error("unable to create pipe:[%d]%s", errno, strerror(errno));
                        
                        // send to client error information
                        n = snprintf(greet_buf, sizeof(greet_buf), "%s ERR %s%s", FAIL_CODE, local_hostname, DATA_END);
                        nwrite = write(connfd, greet_buf, n);
                        log_debug("send client fd[%d]:[%d]%s", connfd, nwrite, greet_buf);
                        
                        continue;
                    }
                    if (pipe(pi2) == -1) {
                        log_error("unable to create pipe:[%d]%s", errno, strerror(errno));
                        
                        close(pi1[0]);
                        close(pi1[1]);
                        pi1[0] = -1;
                        pi1[1] = -1;
                        
                        // send to client error information
                        n = snprintf(greet_buf, sizeof(greet_buf), "%s ERR %s%s", FAIL_CODE, local_hostname, DATA_END);
                        nwrite = write(connfd, greet_buf, n);
                        log_debug("send client fd[%d]:[%d]%s", connfd, nwrite, greet_buf);
                        
                        continue;
                    }
                    log_debug("create pi1[0]:%d pi1[1]:%d", pi1[0], pi1[1]);
                    log_debug("create pi2[0]:%d pi2[1]:%d", pi2[0], pi2[1]);
                    
                    // create unique id
                    n = create_unique_id(childs_t[i].sid, sizeof(childs_t[i].sid));
                    if (n != 16) {
                        log_error("create unique id fail");
                        
                        close(pi1[0]);
                        close(pi1[1]);
                        close(pi2[0]);
                        close(pi2[1]);
                        
                        // send to client error information
                        n = snprintf(greet_buf, sizeof(greet_buf), "%s SYSTEM ERR %s%s", FAIL_CODE, local_hostname, DATA_END);
                        nwrite = write(connfd, greet_buf, n);
                        log_debug("send client fd[%d]:[%d]%s", connfd, nwrite, greet_buf);
                        
                        continue;
                    }
                    log_debug("create mid:%s", childs_t[i].sid);
                    
                    // 当程序执行exec函数时本fd将被系统自动关闭,表示不传递给exec创建的新进程
                    fcntl(pi1[1], F_SETFD, FD_CLOEXEC);
                    fcntl(pi2[0], F_SETFD, FD_CLOEXEC);
                    fcntl(listen_fd, F_SETFD, FD_CLOEXEC);
                    
                    
                    int f = fork();
                    if (f < 0) {
                        log_error("fork fail:[%d]%s", errno, strerror(errno));
                        
                        close(pi1[0]);
                        close(pi1[1]);
                        pi1[0] = -1;
                        pi1[1] = -1;
                        
                        close(pi2[0]);
                        close(pi2[1]);
                        pi2[0] = -1;
                        pi2[1] = -1;
                        
                        // send to client error information
                        n = snprintf(greet_buf, sizeof(greet_buf), "%s SYSTEM ERR %s%s", FAIL_CODE, local_hostname, DATA_END);
                        nwrite = write(connfd, greet_buf, n);
                        log_debug("send client fd[%d]:[%d]%s", connfd, nwrite, greet_buf);
                        
                        continue;
                    
                    } else if (f == 0) {
                        // 子进程
                        close(pi1[1]);
                        close(pi2[0]);
                        pi1[1] = -1;
                        pi2[0] = -1;
                        
                        close(listen_fd);
                        listen_fd = -1;
                        
                        if (fd_move(2, connfd) == -1) {
                            log_error("%s fd_move(2, %d) failed:[%d]%s", childs_t[i].sid, connfd, errno, strerror(errno));
                            _exit(111);
                        }
                        
                        // read from 0
                        if (fd_move(0, pi1[0])) {
                            log_error("%s fd_move(0, %d) failed:[%d]%s", childs_t[i].sid, pi1[0], errno, strerror(errno));
                            _exit(111);
                        }
                        
                        // write to 1
                        if (fd_move(1, pi2[1])) {
                            log_error("%s fd_move(1, %d) failed:[%d]%s", childs_t[i].sid, pi2[1], errno, strerror(errno));
                            _exit(111);
                        }
                        
                        // lunach child program
                        char exe_sid[MAX_LINE] = {0};
                        char exe_cfg[MAX_LINE] = {0};
                        char exe_remote[MAX_LINE] = {0};
                        
                        snprintf(exe_sid, sizeof(exe_sid), "-m%s", childs_t[i].sid);
                        snprintf(exe_cfg, sizeof(exe_cfg), "-c%s", config_t.child_cf);
                        snprintf(exe_remote, sizeof(exe_remote), "-r%s:%s", childs_t[i].client_info.ip, childs_t[i].client_info.port);
                        
                        char *args[5];
                        args[0] = config_t.child_prog;
                        args[1] = exe_sid;
                        args[2] = exe_cfg;
                        args[3] = exe_remote;
                        args[4] = 0;
                        
						char log_exec[MAX_LINE*3] = {0};
						char *plog_exec = log_exec;
						int len = 0;
						int i=0;
						while (args[i] != 0) {
							int n = snprintf(plog_exec + len, sizeof(log_exec) - len, "%s ", args[i]);
							len += n;	
							i++;
						}

                        log_info("Exec:[%s]", log_exec);
                        
                        if (execvp(*args, args) == -1) {
                            log_error("execvp fail:[%d]%s", errno, strerror(errno));
                            _exit(111);
                        }
                        
                        _exit(100);
                        
                    }
                    
                    // 父进程
                    log_debug("add child index:%d pid:%lu", i, f);
                    childs_t[i].pid = f;
                    
                    close(pi1[0]);
                    close(pi2[1]);
                    pi1[0] = -1;
                    pi2[1] = -1;
                    
                    close(connfd);
                    connfd = -1;
                    
                    childs_t[i].pfd_r = pi2[0];
                    childs_t[i].pfd_w = pi1[1];
                    
                    if (set_nonblocking(childs_t[i].pfd_r)) {
                        log_error("set nonblocking fd[%d] fail", childs_t[i].pfd_r);
                    }
                    
                    struct epoll_event pipe_r_ev;
                    pipe_r_ev.events = EPOLLIN | EPOLLET;
                    pipe_r_ev.data.fd = childs_t[i].pfd_r;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipe_r_ev.data.fd, &pipe_r_ev) == -1) {
                        log_error("epoll_ctl client fd[%d] EPOLL_CTL_ADD failed:[%d]%s", pipe_r_ev.data.fd, errno, strerror(errno));
                    }
                    log_debug("epoll_add fd[%d]", pipe_r_ev.data.fd);
                    
                    epoll_num_running++;
                    
                } else if (connfd == -1) {
                    if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR) {
                        log_error("accept failed:[%d]%s", errno, strerror(errno));
                    }
                    
                    continue;
                }
                
            } else if (epoll_evts[epoll_i].events & EPOLLIN) {
                // 有可读事件从子进程过来
                int idx = get_idx_with_sockfd(evt_fd);
                if (idx < 0) {
                    log_error("get_idx_with_sockfd(%d) fail, so not process", evt_fd);
                    continue;
                }
                log_debug("%s get event EPOLLIN: epoll_i[%d] fd[%d] get fd[%d], used[%d]", childs_t[idx].sid, epoll_i, epoll_evts[epoll_i].data.fd, childs_t[idx].pfd_r, childs_t[idx].used);
                
                // 读取内容
                char cbuf[MAX_LINE] = {0};
                nread = read(childs_t[idx].pfd_r, cbuf, sizeof(cbuf));
                log_debug("read buf:'[%d]%s' from child", n, cbuf);

                
            } else if ((epoll_evts[epoll_i].events & EPOLLHUP)
                       && (epoll_evts[epoll_i].data.fd != listen_fd)) {
                // 有子进程退出
                int idx = get_idx_with_sockfd(evt_fd);
                if (idx < 0) {
                    log_error("get_idx_with_sockfd(%d) fail, so not process", evt_fd);
                    
                    continue;
                }
                log_debug("%s get event EPOLLHUP: epoll_i[%d] fd[%d] get fd[%d], used[%d]", childs_t[idx].sid, epoll_i, epoll_evts[epoll_i].data.fd, childs_t[idx].pfd_r, childs_t[idx].used);
                
                epoll_delete_evt(epoll_fd, childs_t[idx].pfd_r);
                
                // 子进程清理
                clean_child_with_idx(idx);
                
                continue;
                
            }
            
        }
        
        sig_childunblock();
    }
    
    close(epoll_fd);
    close(listen_fd);
    
    epoll_fd = -1;
    listen_fd = -1;

	if (childs_t != NULL) {
		free(childs_t);
		childs_t = NULL;
	}

	if (epoll_evts != NULL) {
		free(epoll_evts);
		epoll_evts = NULL;
	}
    
    log_info("I'm finish");
    
    return 0;
    
}


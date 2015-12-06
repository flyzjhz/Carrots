//
//  ctutils.c
//  
//
//  Created by SongJian on 15/12/6.
//
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <strings.h>
#include <sys/param.h>

#include "ctlog.h"
#include "ctutils.h"

#define MAX_LINE		1024

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


// 检查目录是否存在,如不存在则创建
int is_dir_exits(char *path)
{
    char dir_name[MAX_LINE] = {0};
    strcpy(dir_name, path);
    
    int i, len = strlen(dir_name);
    if (dir_name[len - 1] != '/')
        strcat(dir_name, "/");
    
    for (i = 1; i<len; i++) {
        if (dir_name[i] == '/') {
            dir_name[i] = '\0';
            if ( access(dir_name, F_OK) != 0 ) {
                umask(0);
                if ( mkdir(dir_name, 0777) == -1) {
                    log_error("create dir:%s fail:%s", dir_name, strerror(errno));
                    return 1;
                }
            }
            
            dir_name[i] = '/';
        }
    }
    
    return 0;
}


/**
 *  判断是否主进程需要退出.
 *  计算方法: 被分析文件的最后一次修改时间为一天前，则退出
 *
 *  @param file_fd   被分析文件的文件描述符
 *  @param file_name 被分析文件的文件名
 *
 *  @return 0:不退出   1:退出
 */
int is_need_master_exit(int file_fd, char *file_name)
{
    struct stat st = {0};
    if (fstat(file_fd, &st) == -1) {
        printf("stat file:%s fail:%m\n", file_name);
        log_error("stat file:%s fail:%m", file_name);
        return 0;
    }
    if (time(NULL) > (st.st_mtime + (3600 * 24))) {
        return 1;
    }
    
    return  0;
}




void sig_catch( int sig, void (*f) () )
{
    struct sigaction sa; 
    sa.sa_handler = f;
    sa.sa_flags   = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(sig, &sa, (struct sigaction *) 0); 
}


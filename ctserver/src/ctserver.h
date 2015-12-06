//
//  ctserver.h
//
//
//  Created by SongJian on 15/12/2.
//
//

#ifndef ctserver_h
#define ctserver_h

#include <stdio.h>

#define MAX_LINE        1024
#define BUF_SIZE        8192

#define DEF_BINDPORT    "5050"
#define DEF_LOGLEVEL    "info"
#define DEF_CHDIRPATH   "/tmp"
#define DEF_MAXCHILDS   "512"
#define DEF_CHILDPROG   "./child"
#define DEF_CHILDINI    "./child_cf.ini"


// config file struct
struct config_st
{
    char bind_port[10];
    char log_level[MAX_LINE];
    char chdir_path[MAX_LINE];
    char max_childs[MAX_LINE];
    char child_prog[MAX_LINE];
    char child_cf[MAX_LINE];
};


struct client_st
{
    int fd;
    char ip[20];
    char port[20];
};

struct childs_st
{
    int used;
    int pid;
    char sid[MAX_LINE];
    int pfd_r;      // 从子进程读
    int pfd_w;      // 向子进程写
    
    // ------
    struct client_st client_info;
};

#endif /* ctserver_h */

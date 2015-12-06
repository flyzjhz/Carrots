//
//  ctmaster.h
//  
//
//  Created by SongJian on 15/12/6.
//
//

#ifndef ctmaster_h
#define ctmaster_h

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <errno.h>
#include <getopt.h>

#define MAX_LINE        1024
#define BUF_SIZE        4096

#define OFFSET_FILENAME	"read.offset"

struct config_st {
    char log_level[MAX_LINE];
    char process_file[MAX_LINE];
    char chdir_path[MAX_LINE];
    char offset_path[MAX_LINE];
    char wait_sleep[MAX_LINE];
    char max_childs[MAX_LINE];
};

struct childs_st
{
    int used;
    int pid;
};

struct file_info_t {
    int file_rindex_fd;				// 保存被读取文件当前偏移量的文件描述符
    char *file_rindex_offset_ptr;	// 保存偏移量，用于共享内存到文件
    off_t rindex_offset;			// 保存偏移量
};


#endif /* ctmaster_h */

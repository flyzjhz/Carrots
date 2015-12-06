//
//  ctchild.h
//  
//
//  Created by SongJian on 15/12/4.
//
//

#ifndef ctchild_h
#define ctchild_h

#include <stdio.h>

#define MAX_LINE        1024
#define BUF_SIZE        8192    // 8K
#define MAX_BLOCK_SIZE  262144  // 256K

#define DEF_LOGLEVEL    "8"
#define DEF_CONNTIMEOUT "3"
#define DEF_RWTIMEOUT   "5"


// config file struct
struct config_st
{
    char log_level[MAX_LINE];
    char conntimeout[MAX_LINE];
    char rwtimeout[MAX_LINE];
    char buf_size[MAX_LINE];
    char max_blocksize[MAX_LINE];
};

#endif /* ctchild_h */

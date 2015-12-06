//
//  ctutils.h
//  
//
//  Created by SongJian on 15/12/6.
//
//

#ifndef ctutils_h
#define ctutils_h

#include <stdio.h>

void create_daemon(char *chdir_path);

int is_dir_exits(char *path);

int is_need_master_exit(int file_fd, char *file_name);

#endif /* ctutils_h */

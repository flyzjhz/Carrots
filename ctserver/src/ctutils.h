//
//  ctutils.h
//  
//
//  Created by SongJian on 15/12/3.
//
//

#ifndef ctutils_h
#define ctutils_h

#include <stdio.h>



int set_nonblocking(int fd);

/**
 *  使用UUID，生成唯一ID
 *
 *  @param mid      返回的id buffer
 *  @param mid_size id buffer 大小
 *
 *  @return 16:成功  其它:失败
 */
int create_unique_id(char *mid, size_t mid_size);

int fd_copy(int to, int from);
int fd_move(int to, int from);

void sig_catch( int sig, void (*f) () );

void sig_block(int sig);

void sig_unblock(int sig);

void sig_pipeignore();

void sig_childblock();

void sig_childunblock();


void byte_copy(register char *to, register unsigned int n, register char *from);

#endif /* ctutils_h */

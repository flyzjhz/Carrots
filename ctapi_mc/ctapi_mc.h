//
//  ctapi_mc.h
//  
//
//  Created by SongJian on 15/12/8.
//
//

#ifndef ctapi_mc_h
#define ctapi_mc_h

#include <stdio.h>

int set_mc(char *mc_ip, int mc_port, int mc_timeout, char *mc_key, char *mc_value);

void *get_mc(char *mc_ip, int mc_port, int mc_timeout, char *mc_key);

int delete_mc(char *mc_ip, int mc_port, int mc_timeout, char *mc_key);

#endif /* ctapi_mc_h */

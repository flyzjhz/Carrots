//
//  ctapi_mc.c
//  
//
//  Created by SongJian on 15/12/8.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <libmemcached/memcached.h>

#include "ctlog.h"

#include "ctapi_mc.h"


/**
 *  设置key->value到MC中
 *
 *  @param mc_ip      mc 地址
 *  @param mc_port    mc 端口
 *  @param mc_timeout mc 超时时间
 *  @param mc_key     key
 *  @param mc_value   value
 *
 *  @return 0:succ  1:fail
 */
int set_mc(char *mc_ip, int mc_port, int mc_timeout, char *mc_key, char *mc_value, time_t expiration)
{
    size_t nval = 0;
    uint32_t flag = 0;
    char *result = NULL;
    int ret = 0;
    
    memcached_st        *memc = memcached_create(NULL);
    memcached_return    mrc;
    memcached_server_st *mc_servers;
    
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, mc_timeout);
    
    mc_servers= memcached_server_list_append(NULL, mc_ip, mc_port, &mrc);
    log_debug("memcached_server_list_append[%d]:%s:%d", mrc, mc_ip, mc_port);
    
    if (MEMCACHED_SUCCESS == mrc) {
        mrc= memcached_server_push(memc, mc_servers);
        memcached_server_list_free(mc_servers);
        if (MEMCACHED_SUCCESS == mrc) {
            char mc_value_len = strlen(mc_value);
            mrc = memcached_set(memc, mc_key, strlen(mc_key), mc_value, mc_value_len, expiration, (uint32_t)flag);
            if (MEMCACHED_SUCCESS == mrc) {
                log_debug("set mc key:%s val:%s succ", mc_key, mc_value);
                ret = 0;;
            } else {
                log_error("set mc key:%s val:%s failed:%s", mc_key, mc_value, memcached_strerror(memc, mrc));
                ret = 1;
            }
            
            memcached_free(memc);
            return ret;
        }
    }
    
    log_error("set_to_mc:%s:%d connect fail:%s", mc_ip, mc_port, memcached_strerror(memc, mrc));
    
    memcached_free(memc);
    
    return 1;
}


/**
 *  从mc中获取key的值
 *
 *  @param mc_ip         mc 地址
 *  @param mc_port       mc 端口
 *  @param mc_timeout    mc 超时时间
 *  @param mc_key        需要获取值的key
 *  @param mc_value      获取的结果
 *  @param mc_value_size 获取结果的buffer长度
 *
 *  @return NULL:失败
 */
void *get_mc(char *mc_ip, int mc_port, int mc_timeout, char *mc_key)
{
    size_t nval = 0;
    uint32_t flag = 0;
    char *result = NULL;
    //int ret = 0;
    
    memcached_st        *memc = memcached_create(NULL);
    memcached_return    mrc;
    memcached_server_st *mc_servers;
    
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, mc_timeout);
    
    mc_servers= memcached_server_list_append(NULL, mc_ip, mc_port, &mrc);
    log_debug("memcached_server_list_append[%d]:%s:%d", mrc, mc_ip, mc_port);
    
    if (MEMCACHED_SUCCESS == mrc) {
        mrc= memcached_server_push(memc, mc_servers);
        memcached_server_list_free(mc_servers);
        if (MEMCACHED_SUCCESS == mrc) {
            result = memcached_get(memc, mc_key, strlen(mc_key), (size_t *)&nval, &flag, &mrc);
            if (MEMCACHED_SUCCESS == mrc) {
                log_debug("get mc key:%s val:%s succ", mc_key, result);
                
            } else {
                log_error("get mc key:%s ailed:%s", mc_key, memcached_strerror(memc, mrc)); 
            }
            
            memcached_free(memc);
            
            return result;
        }
    }
    
    log_error("set_to_mc:%s:%d connect fail:%s", mc_ip, mc_port, memcached_strerror(memc, mrc));
    
    memcached_free(memc);
    
    return result;
}



/**
 *  从mc中删除一个key
 *
 *  @param mc_ip      mc 地址
 *  @param mc_port    mc 端口
 *  @param mc_timeout mc 超时时间
 *  @param mc_key     需要被删除的key
 *
 *  @return 0:succ 1:key not found 2:fail
 */
int delete_mc(char *mc_ip, int mc_port, int mc_timeout, char *mc_key)
{
    size_t nval = 0;
    uint32_t flag = 0;
    char *result = NULL;
    int ret = 0;
    
    memcached_st        *memc = memcached_create(NULL);
    memcached_return    mrc;
    memcached_server_st *mc_servers;
    
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, mc_timeout);
    
    mc_servers= memcached_server_list_append(NULL, mc_ip, mc_port, &mrc);
    log_debug("memcached_server_list_append[%d]:%s:%d", mrc, mc_ip, mc_port);
    
    if (MEMCACHED_SUCCESS == mrc) {
        mrc= memcached_server_push(memc, mc_servers);
        memcached_server_list_free(mc_servers);
        if (MEMCACHED_SUCCESS == mrc) {
            mrc = memcached_delete(memc, mc_key, strlen(mc_key), 0);
            if (MEMCACHED_SUCCESS == mrc) {
                log_debug("delete mc key:%s succ", mc_key);
                ret = 0;
			} else if (MEMCACHED_NOTFOUND == mrc) {
                log_debug("delete mc key:%s succ. key not found", mc_key);
                ret = 1;
            } else {
                log_error("delete mc key:%s failed:%s", mc_key, memcached_strerror(memc, mrc));
                ret = 2;
            }
            
            memcached_free(memc);
            return ret;
        }
    }
    
    log_error("set_to_mc:%s:%d connect fail:%s", mc_ip, mc_port, memcached_strerror(memc, mrc));
    
    memcached_free(memc);
    
    return 1;
}



#ifdef APIMC_TEST

int main(int argc, char **argv)
{
    char *mc_server = argv[1];
    char *mc_port = argv[2];
    char *mc_timeout = argv[3];
    
    // set mc
    char *key = "name";
    char *value = "kyosold@qq.com";
    
    int succ = set_mc(mc_server, atoi(mc_port), atoi(mc_timeout), key, value);
    if (succ == 0) {
        printf("set mc succ:%s => %s\n", key, value);
    } else {
        printf("set mc fail:%s => %s\n", key, value);
    }
    
    // get mc
    char *pvalue = get_mc(mc_server, atoi(mc_port), atoi(mc_timeout), key);
    if (pvalue) {
        printf("get mc succ:%s => %s\n", key, pvalue);
        
        free(pvalue);
        pvalue = NULL;
    } else {
        printf("get mc fail:%s\n", key);
    }
    
    // delete mc
    succ = delete_mc(mc_server, atoi(mc_port), atoi(mc_timeout), key);
    if (succ == 0) {
        printf("delete mc succ:%s\n", key);
    } else {
        printf("delete mc fail:%s\n", key);
    }
    
    return 0;
}

#endif


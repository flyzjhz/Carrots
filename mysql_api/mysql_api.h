//
//  mysql_api.h
//
//
//  Created by SongJian on 15/11/10.
//
//

#ifndef ____mysql_api__
#define ____mysql_api__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <mysql.h>
#include <errmsg.h>
#include <mysqld_error.h>


#define MYSQL_DEBUG

#define MYSQL_CONN_NAME_LEN     128
#define MYSQL_CONN_TIMEOUT      3
#define MYSQL_CHARSET_NAME      "utf8"
#define MYSQL_CONN_RETRY        0x1


typedef struct _mysql_handle {
    MYSQL *msp;
    char db_host[MYSQL_CONN_NAME_LEN + 1];
    char db_user[MYSQL_CONN_NAME_LEN + 1];
    char db_pass[MYSQL_CONN_NAME_LEN + 1];
    char db_name[MYSQL_CONN_NAME_LEN + 1];
    unsigned int db_port;
    unsigned int db_conn_timeout;
} MYSQL_CONN;






MYSQL_CONN *ct_mysql_connect(char *host, int port, char *user, char *pass, char *name, int conn_timeout);
void ct_mysql_disconnect(MYSQL_CONN *mydb);
int ct_mysql_query(MYSQL_CONN *handle, char *sql_string, int sql_len, int options);
MYSQL_RES *ct_mysql_store_result(MYSQL_CONN *handle, int options);

#endif /* defined(____mysql_api__) */

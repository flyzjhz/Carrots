//
//  mysql_api.c
//
//
//  Created by SongJian on 15/11/10.
//
//

#include "mysql_api.h"


MYSQL_CONN *_mysql_init(const char *db_host, const char *db_user, const char *db_pass,
                        const char *db_name, unsigned int db_port, unsigned int db_conn_timeout);

unsigned long _mysql_real_escape_string(MYSQL_CONN *handle, char *to, unsigned long to_len,
                                        const char *from, unsigned long from_len);

int _mysql_query(MYSQL_CONN *handle, const char *sql_string, int sql_len, int options);

MYSQL_RES *_mysql_store_result(MYSQL_CONN *handle, int options);
int _mysql_options(MYSQL_CONN *handle, int option, const void *arg);
int _mysql_set_server_option(MYSQL_CONN *handle, enum enum_mysql_set_option option);
int _mysql_conn(MYSQL_CONN *handle);
int _mysql_destroy(MYSQL_CONN *handle);


#define mysql_get_handle(handle)        (handle->msp)
#define mysql_get_host(handle)          (handle->db_host)
#define mysql_get_user(handle)          (handle->db_user)
#define mysql_get_pass(handle)          (handle->db_pass)
#define mysql_get_name(handle)          (handle->db_name)
#define mysql_get_port(handle)          (handle->db_port)
#define mysql_get_conn_timeout(handle)  (handle->db_conn_timeout);


int _mysql_conn(MYSQL_CONN *handle)
{
    MYSQL *ret_myql = NULL;
    MYSQL *msp = NULL;
    int retval;
    uint32_t timeout = MYSQL_CONN_TIMEOUT;

    if (handle == NULL) {
        return -1;
    }

    if (handle->db_conn_timeout > 0) {
        timeout = handle->db_conn_timeout;
    }

    /* init mysql lib */
    msp = mysql_init(NULL);
    if (msp == NULL) {
        return -1;
    }

    retval = mysql_options(msp, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)(&timeout));
    if (retval != 0) {
        printf("fail to set option:%s\n", mysql_error(msp));

        mysql_close(msp);
        return -1;
    }

    retval = mysql_options(msp, MYSQL_SET_CHARSET_NAME, MYSQL_CHARSET_NAME);
    if (retval != 0) {
        printf("fail to set option:%s\n", mysql_error(msp));

        mysql_close(msp);
        return -1;
    }

    handle->msp = msp;

    ret_myql = mysql_real_connect(handle->msp,
                                  handle->db_host,
                                  handle->db_user,
                                  handle->db_pass,
                                  handle->db_name,
                                  handle->db_port,
                                  NULL,
                                  CLIENT_INTERACTIVE | CLIENT_MULTI_STATEMENTS);
    if (ret_myql == NULL) {
        printf("fail to connect:%s\n", mysql_error(handle->msp));

        mysql_close(handle->msp);
        handle->msp = NULL;
        return -2;
    }

    mysql_autocommit(handle->msp, 1);

    return 0;
}

MYSQL_CONN *_mysql_init(const char *db_host, const char *db_user, const char *db_pass,
                        const char *db_name, unsigned int db_port, unsigned int db_conn_timeout)
{
    MYSQL_CONN *handle;

    if (db_host == NULL || db_user == NULL || db_pass == NULL || db_name == NULL) {
        return NULL;
    }

    /* create a mysql conn handle */
    handle = (MYSQL_CONN *)calloc(1, sizeof(MYSQL_CONN));
    if (handle == NULL) {
        return NULL;
    }

    /* save the mysql information */
    strncpy(handle->db_host, db_host, MYSQL_CONN_NAME_LEN);
    strncpy(handle->db_user, db_user, MYSQL_CONN_NAME_LEN);
    strncpy(handle->db_pass, db_pass, MYSQL_CONN_NAME_LEN);
    strncpy(handle->db_name, db_name, MYSQL_CONN_NAME_LEN);
    handle->db_port = db_port;
    handle->db_conn_timeout = db_conn_timeout;

    /* real connect to mysql server */
    if (_mysql_conn(handle) != 0) {
        if (handle != NULL) {
            free(handle);
            handle = NULL;
        }
        return NULL;
    }

    return handle;
}

int _mysql_query(MYSQL_CONN *handle, const char *sql_string, int sql_len, int options)
{
    int retval = 0;
    int err;
    int retry_flag = 0;

    if (handle == NULL || sql_string == NULL) {
        return -1;
    }

    if (handle->msp == NULL) {
        if (_mysql_conn(handle) != 0) {
            printf("_mysql_store_result: MySQL can not be reconnected!.\n");
            return -1;
        }
    }

    for (; ; ) {
        retval = mysql_real_query(mysql_get_handle(handle), sql_string, sql_len);
        if (retval != 0) {
            printf("mysql query error:%d:%s\n", retval, mysql_error(mysql_get_handle(handle)));
            err = mysql_errno(mysql_get_handle(handle));

            /* syntax error */
            if (err >= ER_ERROR_FIRST && err <= ER_ERROR_LAST) {
                if (err == ER_SYNTAX_ERROR) {
                    printf("SQL syntax error.\n");
                }
                return err;

            } else if (err == CR_SERVER_GONE_ERROR || err == CR_SERVER_LOST) {
                /* connection error */
                if ((options & MYSQL_CONN_RETRY) != MYSQL_CONN_RETRY) {
                    retry_flag = 1;
                }

                /* Disconnected again, never retry */
                if (retry_flag) {
                    break;
                }

                retry_flag = 1;
                printf("Try to reconnect to mysql ...\n");

                /* close for a new connection */
                mysql_close(handle->msp);

                if (_mysql_conn(handle) != 0) {
                    printf("Fatal error: MySQL can not be reconnected!.\n");
                    handle->msp = NULL;

                    return -1;

                } else {
                    continue;
                }

            } else {
                /* other fatal error */
                printf("Mysql Unknow error. mysql failed.\n");

                return -1;
            }

        } else {
            break;
        }
    }

    return 0;
}

MYSQL_RES *_mysql_store_result(MYSQL_CONN *handle, int options)
{
    MYSQL_RES *result = NULL;
    int err;
    int retry_flag = 0;

    if (handle == NULL) {
        return NULL;
    }

    if (handle->msp == NULL) {
        if (_mysql_conn(handle) != 0) {
            printf("_mysql_store_result: MySQL can not be reconnected!.\n");
            return NULL;
        }
    }

    for (; handle->msp != NULL; ) {
        result = mysql_store_result(mysql_get_handle(handle));

        if (result == NULL) {
            err = mysql_errno(mysql_get_handle(handle));

            /* syntax error */
            if (err >= ER_ERROR_FIRST && err <= ER_ERROR_LAST) {
                break;

            } else if (err == CR_SERVER_GONE_ERROR || err == CR_SERVER_LOST) {
                /* connection error */
                if ((options & MYSQL_CONN_RETRY) != MYSQL_CONN_RETRY)
                    retry_flag = 1;

                /* Disconnected again, never retry */
                if (retry_flag)
                    break;

                /* Reconnect it again */
                retry_flag = 1;
                mysql_close(handle->msp);

                if (_mysql_conn(handle) != 0) {
                    printf("Fatal error: MySQL can not be reconnected!.\n");
                    handle->msp = NULL;
                    return NULL;
                }
                else {
                    continue;
                }

            } else {
                /* other fatal error */
                if (err)
                    printf("Mysql Unknow error. mysql failed.\n");

                return NULL;
            }

        } else {
            break;
        }
    }

    return result;
}

int _mysql_options(MYSQL_CONN *handle, int option, const void *arg)
{
    if (handle == NULL || arg == NULL)
        return -1;

    if (handle->msp == NULL) {
        /* reconnect mysql server */
        if (_mysql_conn(handle) != 0) {
            printf("__mysql_options: MySQL can not be reconnected!.\n");
            return -2;
        }
    }

    if (handle->msp != NULL) {
        mysql_options(handle->msp, option, arg);
        return 0;
    }

    return -2;
}

int _mysql_set_server_option(MYSQL_CONN *handle, enum enum_mysql_set_option option)
{
    if (handle == NULL)
        return -1;

    if (handle->msp == NULL) {
        /* reconnect mysql server */
        if (_mysql_conn(handle) != 0) {
            printf("__mysql_options: MySQL can not be reconnected!.\n");
            return -2;
        }
    }

    if (handle->msp != NULL) {
        mysql_set_server_option(handle->msp, option);
        return 0;
    }

    return -2;
}

unsigned long _mysql_real_escape_string(MYSQL_CONN *handle, char *to, unsigned long to_len,
                                        const char *from, unsigned long from_len)
{
    return mysql_real_escape_string(handle->msp, to, from, from_len);
}

int _mysql_destroy(MYSQL_CONN *handle)
{
    if (handle == NULL) {
        return -1;
    }

    mysql_close(handle->msp);
    handle->msp = NULL;

    free(handle);
    handle = NULL;

    return 0;
}




MYSQL_CONN *ct_mysql_connect(char *host, int port, char *user, char *pass, char *name, int db_conn_timeout)
{
    MYSQL_CONN *mydb = _mysql_init(host, user, pass, name, port, db_conn_timeout);
    if (mydb == NULL) {
        printf("_mysql_init error, host:%s:%d, user:%s, pass:%s, dbname:%s, timeout:%d\n", host, port, user, pass, name, db_conn_timeout);
        return NULL;
    }

    return mydb;
}

void ct_mysql_disconnect(MYSQL_CONN *mydb)
{
    if (mydb != NULL) {
        _mysql_destroy(mydb);
    }
}

int ct_mysql_query(MYSQL_CONN *handle, char *sql_string, int sql_len, int options)
{
    return _mysql_query(handle, sql_string, sql_len, options);
}

MYSQL_RES *ct_mysql_store_result(MYSQL_CONN *handle, int options)
{
    return _mysql_store_result(handle, options);
}


/* Test Code */
#ifndef TEST
#define user "sales03@tti-mould.sinanet.com"
#define subject "This is test subject"
#define db_host "mysql_host"
#define db_port "mysql_port"
#define db_user "mysql_user"
#define db_pass "mysql_pass"
#define db_name "mysql_db"
#define db_timeout 1

#define MAX_LINE    1024
int main(int argc, char const *argv[]) {
    static MYSQL_CONN *db = NULL;
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    char sql[MAX_LINE] = {0};
    int n = 0;

    db = ct_mysql_connect(db_host, atoi(db_port), db_user, db_pass, db_name, db_timeout);
    if (db == NULL) {
        printf("mysql connect fail:%s\n", db_host);
        return -1;
    }

    // escape special string
    char escape_subject[MAX_LINE] = {0};
    n = mysql_real_escape_string(db->msp, escape_subject, subject, strlen(subject));
    escape_subject[n] = '\0';
    char escape_user[1024] = {0};
    n = mysql_real_escape_string(db->msp, escape_user, user, strlen(user));
    escape_user[n] = '\0';

    // insert or update
    n = snprintf(sql, sizeof(sql), "update mail_info set subject = '%s' where default_email = '%s' limit 1", escape_subject, escape_user);
    if (ct_mysql_query(db, sql, strlen(sql), 1) != 0) {
        printf("ct_mysql_query error, sql:%s\n", sql);
        ct_mysql_disconnect(db);
        return -1;
    }
    if (mysql_affected_rows(db->msp) != 1) {
        printf("update error, sql:%s\n", sql);
        ct_mysql_disconnect(db);
        return -1;
    }

    // select
    n = snprintf(sql, sizeof(sql), "select * from mail_info where default_email = '%s'", escape_user);
    if (ct_mysql_query(db, sql, strlen(sql), 1) != 0) {
        printf("ct_mysql_query error\n");
        ct_mysql_disconnect(db);
        db = NULL;
        return -1;
    }

    res = ct_mysql_store_result(db, 1);
    if (res == NULL) {
        printf("ct_mysql_store_result error\n");
        ct_mysql_disconnect(db);
        db = NULL;
        return -1;
    }

    while ((row = mysql_fetch_row(res)) != NULL) {
        printf("%s %s %s %s %s %s \n",
            row[0], row[1], row[2], row[3], row[4], row[5]);
    }

    mysql_free_result(res);
    ct_mysql_disconnect(db);



    return 0;
}
#endif

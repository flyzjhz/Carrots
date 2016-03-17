//
//  ctapi_curl.c
//  
//
//  Created by SongJian on 16/3/16.
//
//

#include "ctapi_curl.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>

#define BUF_SIZE	10


size_t _recive_header_from_http_api(void *buffer, size_t size, size_t nmemb, void *user_p)
{
	//printf("_recive_header_from_http_api: %s\n", (char *)buffer);

	struct pstr_t *header_t = (struct pstr_t *)user_p;
	//printf("_recive_header_from_http_api: header_t: len[%d] size[%d] str[%p][%s]\n", header_t->len, header_t->size, header_t->str, header_t->str);

	if (header_t->str == NULL) {
		header_t->str = (char *)calloc(1, BUF_SIZE);
		if (header_t->str == NULL) {
			printf("memory error\n");
			return 0;
		}
		header_t->size = BUF_SIZE;
		header_t->len = 0;
	}

	if ( (header_t->size - header_t->len) < ((size * nmemb) + 1) ) {
		unsigned int new_header_size = (header_t->size * 2) + (size * nmemb) + 1;
		char *new_pstr_hr = (char *)realloc(header_t->str, new_header_size);
		if (new_pstr_hr == NULL) {
			printf("memory error\n");
			return 0;
		}
		header_t->str = new_pstr_hr;
		header_t->size = new_header_size;
	}

	memcpy((header_t->str + header_t->len), buffer, (size * nmemb));
	header_t->len += (size * nmemb);
	header_t->str[header_t->len] = '\0';

	return (size * nmemb);
}

size_t _recive_data_from_http_api(void *buffer, size_t size, size_t nmemb, void *user_p)
{
	//printf("_recive_data_from_http_api: %s\n", (char *)buffer);

	struct pstr_t *data_t = (struct pstr_t *)user_p;
	//printf("_recive_data_from_http_api: data_t: len[%d] size[%d] str[%p][%s]\n", data_t->len, data_t->size, data_t->str, data_t->str);

	if (data_t->str == NULL) {
		data_t->str = (char *)calloc(1, BUF_SIZE);
		if (data_t->str == NULL) {
			printf("memory error\n");
			return 0;
		}
		data_t->size = BUF_SIZE;
		data_t->len = 0;
	}

	if ( (data_t->size - data_t->len) < ((size * nmemb) + 1) ) {
		unsigned int new_data_size = (data_t->size * 2) + (size * nmemb) + 1;
		char *new_pstr_da = (char *)realloc(data_t->str, new_data_size);
		if (new_pstr_da == NULL) {
			printf("memory error\n");
			return 0;
		}
		data_t->str = new_pstr_da;
		data_t->size = new_data_size;
	}

	memcpy((data_t->str + data_t->len), buffer, (size * nmemb));
	data_t->len += (size * nmemb);
	data_t->str[data_t->len] = '\0';

	return (size * nmemb);
}


/**
 *  使用curl提交数据
 *
 *  @param url             被提交的URL，需带:http/https
 *  @param method          提交方式: GET/POST
 *  @param post_data       POST的数据，格式为: key1=val1&key2=val2
 *  @param save_cookie_fs  需要保存cookie的文件
 *  @param send_cookie_fs  需要提交给服务器的cookie文件
 *  @param connect_timeout 连接服务器最长时间, 单位:s
 *  @param timeout         函数执行最长时间, 单位:s
 *  @param curl_result_t   获取服务器返回的数据
 *  @param error           出错时的内容
 *  @param error_size      error的buffer长度
 *
 *  @return 0:出错 其它:正常
 */
int submit_data_to_http(char *url, char *method, char *post_data, char *save_cookie_fs, char *send_cookie_fs, unsigned int connect_timeout, unsigned int timeout, struct curl_return_string *curl_result_t, char *error, size_t error_size)
{
    curl_global_init(CURL_GLOBAL_ALL);

    CURL *curl = NULL;
    CURLcode ret;
    char *post_data_urlenc = NULL;

    
    // init curl
    curl = curl_easy_init();
    if (!curl) {
        snprintf(error, error_size, "couldn't init curl");
		goto SUBMIT_DATA_TO_HTTP_FAIL;
    }
    
    // 设置提交方式
    if (strcasecmp(method, "POST") == 0) {
        // 设置方法为Post
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        
        // 设置提交数据
        if (post_data != NULL) {
            // 数据做urlencode
            post_data_urlenc = curl_easy_escape(curl, post_data, strlen(post_data));
            if (post_data) {
                // 设置POST 数据
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data_urlenc);
            }
        }
    }
    
    // 设置SSL
    if (strncasecmp(url, "HTTPS", 5) == 0) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, "./cacert.pem");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    
    // 设置函数执行最长时间
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    // 设置连接服务器最长时间
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connect_timeout);
    
    // 设置保存 cookie 到文件
    if (save_cookie_fs != NULL) {
        curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 1);
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, save_cookie_fs);
    }
    
    // 设置发送的cookie
    if (send_cookie_fs != NULL) {
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, send_cookie_fs);
    }

	// 设置处理接收到头数据的回调函数
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, _recive_header_from_http_api);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &curl_result_t->headers);

    // 设置处理接收到的下载数据的回调函数
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _recive_data_from_http_api);
    // 设置CURLOPT_WRITEFUNCTION回调函数返回的数据指针
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_result_t->data);

    
    ret = curl_easy_perform(curl);
    if (ret != CURLE_OK) {
        snprintf(error, error_size, "curl_easy_perform() failed:%s, url:%s", curl_easy_strerror(ret), url);
		printf("curl_easy_perform() failed:%s, url:%s", curl_easy_strerror(ret), url);

    } else {
		//printf("\n %p --------- Response Headers %d ----------\n", &curl_result_t->headers, curl_result_t->headers.len);
		//printf("%s", header.str);
		//printf("\n %p --------- Response Body %d ----------\n", &curl_result_t->data, curl_result_t->data.len);
		//printf("%s", data.str);
	}

    
SUBMIT_DATA_TO_HTTP_SUCC:
    if (post_data_urlenc != NULL) {
        curl_free(post_data_urlenc);
        post_data_urlenc = NULL;
    }
    
	if (curl) {
    	curl_easy_cleanup(curl);
	}

    curl_global_cleanup();

	return (curl_result_t->headers.len + curl_result_t->data.len);
		

SUBMIT_DATA_TO_HTTP_FAIL:

	if (curl_result_t->headers.str != NULL) {
		free(curl_result_t->headers.str);
		curl_result_t->headers.str = NULL;
		curl_result_t->headers.size = 0;
	}

    
    if (post_data_urlenc != NULL) {
        curl_free(post_data_urlenc);
        post_data_urlenc = NULL;
    }
    
	if (curl) {
    	curl_easy_cleanup(curl);
	}
    curl_global_cleanup();

	return 0;
    
}



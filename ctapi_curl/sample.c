#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "ctapi_curl.h"

int main(int argc, char **argv)
{
	/*char *url = "https://passport.mafengwo.cn/login-popup.html";
	char *method = "POST";
	char *post_data = "passport=kyosold@qq.com&password=metalryu6S";
	char *save_cookie_fs = "./cookie.txt";
	char *send_cookie_fs = NULL;*/

	char *url = "http://www.zimuzu.tv/User/Login/ajaxLogin";
	char *method = "POST";
	char *post_data = "account=kyosold&password=metalryu&remember=1&url_back=http://www.zimuzu.tv/";;
	char *save_cookie_fs = "./cookie_zm.txt";
	char *send_cookie_fs = NULL;

	/*char *url = "http://www.baidu.com";
	char *method = "GET";
	char *post_data = NULL;
	char *save_cookie_fs = NULL;
	char *send_cookie_fs = NULL;*/

	struct curl_return_string curl_return_t;
	curl_return_t.headers.str = NULL;
	curl_return_t.headers.len = 0;
	curl_return_t.headers.size = 0;
	curl_return_t.data.str = NULL;
	curl_return_t.data.len = 0;
	curl_return_t.data.size = 0;

	char error[1024] = {0};

	int ret = submit_data_to_http(url, method, post_data, save_cookie_fs, send_cookie_fs, 10, 10, &curl_return_t, error, sizeof(error));
	if (ret == 0) {
		printf("fail:%s\n", error);
	} else {
		printf("succ:\n", ret);
		printf("------ Response Headers %d ------\n", curl_return_t.headers.len);
		if (curl_return_t.headers.str != NULL) {
			printf("%s\n", curl_return_t.headers.str);
		}
		
		printf("\n------ Response Body %d ------\n", curl_return_t.data.len);
		if (curl_return_t.data.str != NULL) {
			printf("%s\n", curl_return_t.data.str);
		}
	}
	

	if (curl_return_t.headers.str != NULL) {
		free(curl_return_t.headers.str);
		curl_return_t.headers.str = NULL;
	}
	curl_return_t.headers.len = 0;
	curl_return_t.headers.size = 0;

	if (curl_return_t.data.str != NULL) {
		free(curl_return_t.data.str);
		curl_return_t.data.str = NULL;
	}
	curl_return_t.data.len = 0;
	curl_return_t.data.size = 0;	

	return 0;
}

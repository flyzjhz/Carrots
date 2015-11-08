confparser
=======

Intro
-----

[confparser] is a library for read and parse config file


Installation
------------

#### Build
```bash
	# Linux
	make -f linux.mk
```

### Usage

Config File: /usr/local/etc/test.ini
```bash
[master]
log_level="info"
max_childs=1
log_file="/var/log/maillog"
conf_path="/usr/home/conf/"

[child]
timeout=180 # 3分钟
mc_host="127.0.0.1"
mc_port="1234"
mc_timeout=28800 # 8小时
```

```bash
	# Import Header Files
	#include "confparser.h"
	#include "dictionary.h"

	void close_config(dictionary *dict)
	{
    	if (dict) {
        	dictionary_del(dict);
        	dict = NULL;
    	}   
	}

	dictionary *read_config(char *conf_file)
	{
		int i = 0;
		int ret = 0;

		dictionary *dict = open_conf_file(conf_file);
		if (dict == NULL) {
			log_error("parse config file fail when open_conf_file file:%s", conf_file);
			return NULL;
		}   

		# NOTE: You Just Can Use parse_conf_file one time, Same Like Below
		// ---- Master ----
		struct conf_int_config master_int_all_array[] = { 
			{"max_childs", &max_child_num},
			{0,0}
		};  

		struct conf_str_config master_str_all_array[] = { 
			{"log_level", plog_level},
			{"log_file", rfile},
			{"conf_path", cpath},
			{0,0}
		};  

		ret = parse_conf_file(dict, "master", master_int_all_array, master_str_all_array);
		if (ret != 0) {
			log_error("parse config file fail when parse 'master'");

			close_config(dict);
    		return NULL;

		}   

		return dict;
	}


	int main(int argc, char **argv) {

		// config file path
		char *cfile = argv[1];

		dictionary *dict_conf = NULL;

		// read config and return dictonary
    	dict_conf = read_config(cfile);
    	if (dict_conf == NULL) {
        	log_error("read_config fail:%s", cfile);
        	exit(1);
    	}

		// You Can Use Anywhere Or Anytimes Like Below To Read Config File Content
		// ----- Child -----
		char *pchild_timeout = dictionary_get(dict, "child:timeout", NULL);
		if (pchild_timeout != NULL) {
			child_timeout = atoi(pchild_timeout);
		}
		char *pmc_host = dictionary_get(dict, "child:mc_host", NULL);
		if (pmc_host != NULL) {
			snprintf(mc_host, sizeof(mc_host), "%s", pmc_host);
		}
		char *pmc_port = dictionary_get(dict, "child:mc_port", NULL);
		if (pmc_port != NULL) {
			snprintf(mc_port, sizeof(mc_port), "%s", pmc_port);
		}
		char *pmc_timeout = dictionary_get(dict, "child:mc_timeout", NULL);
		if (pmc_timeout != NULL) {
			mc_timeout = atoi(pmc_timeout);
		}
		// ----- Child -----

		
		close_config(dict);

		return 0;
	}
		
```

3. Build
```bash
	# Create Static Library 'libconfparser.a'
	make -f linux.mk
	# Build Your Progress
	gcc -g -o test test.c ./confparser/libconfparser.a -I./confparser/
```




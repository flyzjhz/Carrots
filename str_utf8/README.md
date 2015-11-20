str_utf8
=======

Intro
-----

[str_utf8] is get a string with utf8 charset

If you have a string, you want get some char from left and you don't care charset,
You need use this, it just copy some utf8 charset char for you.


Installation
------------

### Usage

1. if you want to encode your voice to small size file
```bash
	char *in = "hello, 你好,谢谢";
	unsigned int n = 10;
	char out[n * 3 + 1] = {0};
	char *p_des = NULL;

	char *p_des = utf8_strncpy(out, utf8_ltrim(in), n);
	if (*p_des != '\0' && *out != '\0') {
		printf("out:%s\n", out);
	}

```

3. Build
```bash
	gcc -g -o test test.c str_utf8.c
```




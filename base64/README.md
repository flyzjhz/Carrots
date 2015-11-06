base64
=======

Intro
-----

[base64] is a encode/decode for base64


Installation
------------

#### Build

### Usage

1. if you want to encode your voice to small size file
```bash
	char *in = "test encode this string";
	char *out = NULL;

	size_t b64_att_len = base64_encode_alloc(in, strlen(in), &out);
	printf("%d:%s\n", b64_att_len, out);

	if (out) {
		free(out);
		out = NULL;
	}
```

2. if you want to decode the smaill size audio file to original audio
```bash
	char *in = "dGVzdCBlbmNvZGUgdGhpcyBzdHJpbmc=";
	char *out = NULL;
	size_t outlen = 0;
	
	struct base64_decode_context ctx;
	base64_decode_ctx_init(&ctx);
	bool ret = base64_decode_alloc_ctx(&ctx, in, strlen(in), &out, &outlen);
	printf("%d:%s\n", outlen, out);

	if (out) {
		free(out);
		out = NULL;
	}
```

3. Build
```bash
	gcc -g -o base64 base64.c
```




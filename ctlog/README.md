ctlog
=======

Intro
-----

[ctlog] is function for write log with syslog.


Installation
------------

#### Usage: 
Sames like openlog:
ctlog(const char \*ident, int option, int facility);

```bash
# 1. open maillog
ctlog("log_ident", LOG_PID|LOG_NDELAY, LOG_MAIL);

# 2. set session unique id
snprintf(log_sid, sizeof(log_sid), "tB24U1FL025441");

# 3. set log level
log_level = info;

# 4. write log:
log_info("info log:%s", "hello, world");	# info
log_debug("debug log:%s", "hello, world");  # debug
log_error("error log:%s", "hello, world");	# error

```



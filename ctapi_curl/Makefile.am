AUTOMAKE_OPTIONS=foreign

if BUILD_ctapi_curl
# [动态库]
lib_LTLIBRARIES=libgs_ctapi_curl.la
libgs_ctapi_curl_la_SOURCES=ctapi_curl.c ctapi_curl.h
libgs_ctapi_curl_la_CFLAGS=-g -I./

# 安装头文件到 {$prefix}/include/ctapi_curl/ 下
ctapimcincludedir=$(includedir)/ctapi_curl
ctapimcinclude_HEADERS=ctapi_curl.h

endif

libdir=$(prefix)/lib/ctapi_curl

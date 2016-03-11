//
//  stralloc.c
//  
//
//  Created by SongJian on 16/3/11.
//
//

#include "stralloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


char *_alloc(unsigned int n);
int _alloc_re(char **x, unsigned int m, unsigned int n);
void _alloc_free(char *x);



/**
 *  判断 sa 是否以 s 开头
 *
 *  @param sa 被比较变量
 *  @param s  比较变量
 *
 *  @return <#return value description#>
 */
int stralloc_starts(stralloc *sa, char *s)
{
    int len = strlen(s);
    return ((sa->len >= len) && (strncmp(sa->s, s, len) == 0));
}


#pragma mark - Ready Check Memory
/**
 *  检查 x 内存空间是否足够放下 n 个字节，如果没有则扩大;
 *  如果 x 是空的，则初始化申请一个新的
 *
 *  @param x 被检查的 x
 *  @param n  需要的空间长度
 *
 *  @return 0:失败 其它:成功
 */
int stralloc_ready(register stralloc *x, register unsigned int n)
{
    register unsigned int i;
    if (x->s != NULL) {
        i = x->a;
        if (n > i) {
            x->a = 30 + n + (n >> 3);
            if (_alloc_re(&x->s, i * sizeof(char), x->a * sizeof(char))) {
                return 1;
            }
            x->a = i;
            return 0;
        }
        return 1;
    }
    x->len = 0;
    return !!(x->s = (char*)_alloc((x->a = n) * sizeof(char)));
}


/**
 *  检查 x 中剩余的空间是否足够放下 n 个字节，没有则扩大
 *
 *  @param x <#x description#>
 *  @param n <#n description#>
 *
 *  @return 0:失败 其它:成功
 */
int stralloc_readyplus(register stralloc *x, register unsigned int n)
{
    register unsigned int i;
    if (x->s != NULL) {
        i = x->a;
        n += x->len;
        if (n > i) {
            x->a = 30 + n + (n >> 3);
            if (_alloc_re(&x->s, i * sizeof(char), x->a * sizeof(char))) {
                return 1;
            }
            x->a = i;
            return 0;
        }
        return 1;
    }
    x->len = 0;
    return !!(x->s = (char *)_alloc((x->a = n) * sizeof(char)));
}



#pragma mark - String Copy
/**
 *  从 s 中拷贝 n 个字节到 sa 中。
 *  sa 不足则自动扩展空间并再次拷贝
 *
 *  @param sa 被拷贝的sa
 *  @param s  从s中拷贝
 *  @param n  拷贝前n个字节
 *
 *  @return 0:失败 1:成功
 */
int stralloc_copyb(stralloc *sa, char *s, unsigned int n)
{
    if (!stralloc_ready(sa, n + 1)) {
        return 0;
    }
    
    memcpy(sa->s, s, n);
    sa->len = n;
    sa->s[n] = '\0';     // offensive programming
    
    return 1;
}


/**
 *  拷贝s中字符串到sa中。
 *  仅拷贝s中以\0之前的字符到sa
 *
 *  @param sa 被拷贝的sa
 *  @param s  从s中拷贝
 *
 *  @return 0:失败 1:成功
 */
int stralloc_copys(stralloc *sa, char *s)
{
    return stralloc_copyb(sa, s, strlen(s));
}


/**
 *  拷贝 sa_src 到 sa_dst 中
 *
 *  @param sa_dst <#sa_dst description#>
 *  @param sa_src <#sa_src description#>
 *
 *  @return 0:失败 1:成功
 */
int stralloc_copy(stralloc *sa_dst, stralloc *sa_src)
{
    return stralloc_copyb(sa_dst, sa_src->s, sa_src->len);
}



#pragma mark - String Cat
/**
 *  追加字符串s前n个字符到sa中
 *
 *  @param sa 被添加字符的sa
 *  @param s  源字符串s
 *  @param n  追回前n个字符
 *
 *  @return 0:失败 1:成功
 */
int stralloc_catb(stralloc *sa, char *s, unsigned int n)
{
    if (!sa->s) {
        return stralloc_copyb(sa, s, n);
    }
    if (!stralloc_readyplus(sa, n + 1)) {
        return 0;
    }
    
    memcpy(sa->s + sa->len, s, n);
    sa->len += n;
    sa->s[sa->len] = '\0';      // offensive programming
    
    return 1;
}


/**
 *  追加 s 到 sa 中，s必须是带有'\0'
 *
 *  @param sa 被追加 sa
 *  @param s  必须带有'\0'
 *
 *  @return 0:失败 1:成功
 */
int stralloc_cats(stralloc *sa, char *s)
{
    return stralloc_catb(sa, s, strlen(s));
}


/**
 *  追加 sa_src 到 sa_dst 中
 *
 *  @param sa_dst <#sa_dst description#>
 *  @param sa_src <#sa_src description#>
 *
 *  @return 0:失败 1:成功
 */
int stralloc_cat(stralloc *sa_dst, stralloc *sa_src)
{
    return stralloc_catb(sa_dst, sa_src->s, sa_src->len);
}



#pragma mark - Free
void stralloc_free(register stralloc *x)
{
    if (x && x->s) {
        _alloc_free(x->s);
        x->s = 0;
        x->len = 0;
        x->a = 0;
    }
}



#pragma mark - _Memory Alloc

#define ALIGNMENT   16      // XXX: assuming that this alignment is enough
#define SPACE       4096    // must be multiple of ALIGNMENT

typedef union {
    char irrelevant[ALIGNMENT];
    double d;
} aligned;

static aligned realspace[SPACE / ALIGNMENT];
#define space ((char *)realspace)
static unsigned int avail = SPACE;      // multiple of ALIGNMENT; 0<=avail<=SPACE

char *_alloc(unsigned int n)
{
    char *x;
    n = ALIGNMENT + n - (n & (ALIGNMENT - 1));  // XXX: could overflow
    if (n <= avail) {
        avail -= n;
        return space + avail;
    }
    x = (char *)malloc(n);
    //if (x == NULL) {
    //    errno = error_nomem;
    //}
    return x;
}

void _alloc_free(char *x)
{
    if (x >= space) {
        if (x < space + SPACE) {
            return;     // XXX: assuming that pointers are flat
        }
    }
    free(x);
}

int _alloc_re(char **x, unsigned int m, unsigned int n)
{
    char *y = _alloc(n);
    if (y == NULL) {
        return 0;
    }
    memcpy(y, *x, m);
    _alloc_free(*x);
    *x = y;
    return 1;
}



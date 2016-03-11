//
//  stralloc.h
//  
//
//  Created by SongJian on 16/3/11.
//
//

#ifndef stralloc_h
#define stralloc_h



typedef struct stralloc {
    char *s;
    int len;    // s已使用的空间大小
    int a;      // s的空间大小
} stralloc;

typedef struct stralloc_s {
    stralloc *sa;
    int len;
    int a;
} stralloc_s;




int stralloc_starts(stralloc *sa, char *s);
int stralloc_ready(register stralloc *x, register unsigned int n);
int stralloc_readyplus(register stralloc *x, register unsigned int n);
int stralloc_copyb(stralloc *sa, char *s, unsigned int n);
int stralloc_copys(stralloc *sa, char *s);
int stralloc_copy(stralloc *sa_dst, stralloc *sa_src);
int stralloc_catb(stralloc *sa, char *s, unsigned int n);
int stralloc_cats(stralloc *sa, char *s);
int stralloc_cat(stralloc *sa_dst, stralloc *sa_src);
void stralloc_free(register stralloc *x);




#endif /* stralloc_h */


#ifndef _MFILE_H_
#define _MFILE_H_

#include "queue.h"

#define DEFAULT_DATA_BLOCK_SIZE  8888
typedef void *USER_ALLOCER(size_t);
typedef void USER_FREER(void *);

typedef struct data_block
{
	int data_len;
	char *data;
	int current_write_count;
	int current_read_count;
	TAILQ_ENTRY(data_block) entries;
} DATA_BLOCK;

TAILQ_HEAD(mfile_head, data_block);

/* mfile's handler */
typedef struct mfile
{
	int total_size;
	unsigned int new_header_size;
	struct mfile_head head; /* queue head */
	int block_num; /* data block number */
	DATA_BLOCK *current_write_p;
	DATA_BLOCK *current_read_p;
	int is_full; /* 0: not full, 1:full */
	int is_empty; /* already readed */
	int DATA_BLOCK_SIZE;
	int MAX_BLOCK_NUMBER;
	char ccache;
	USER_ALLOCER *alloc_fun;
	USER_FREER *free_fun;
} MFILE;

MFILE *mopen(int data_block_size, USER_ALLOCER *af, USER_FREER *fe);
void mclose(MFILE *handler);
int mwrite(MFILE *handler, const char *data, int len);
int mwrite_head(MFILE *handler, const char *data, int len);
int mread(MFILE *handler, char *data, int len);
void mseek(MFILE *handler);
void mseek_pos(MFILE *handler, int pos);
char mgetc(MFILE *handler);
int mread_line(MFILE *handler, char *buf, int n);
int mwrite_file(MFILE *handler, int fd);
int msize(MFILE *handler);
unsigned int mfile_new_header_size( MFILE *handler );
int mfile_copy(MFILE* dest, MFILE* src, unsigned int start, unsigned int end);

#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>

#include "mfile.h"
//#define MALLOC_DEBUG
//#ifdef MALLOC_DEBUG
//extern int total_malloc;
//extern int total_free;
//int total_malloc;
//int total_free;
//#endif

/*
* mopen()
* @data_block_size:  block size
* Upon successful completion mopen() return a MFILE pointer.
* Otherwise, NULL is returned
*/
MFILE *
mopen(int data_block_size, USER_ALLOCER *af, USER_FREER *fe)
{
	MFILE *handler;

	if (af == NULL && fe == NULL) {
		handler = malloc(sizeof(MFILE));
		memset(handler, 0, sizeof(MFILE));
		handler->alloc_fun = malloc;
		handler->free_fun = free;
	} else if (af != NULL && fe != NULL) {
		handler = af(sizeof(MFILE));
		memset(handler, 0, sizeof(MFILE));
		handler->free_fun = fe;
		handler->alloc_fun = af;
	} else {
		return NULL;
	}

	if (NULL == handler) {
		fprintf(stderr, "malloc error in mopen\n");
		return NULL ;
	}
#ifdef MALLOC_DEBUG
	++total_malloc;
#endif
	TAILQ_INIT(&handler->head);
	handler->is_full = 1;
	handler->is_empty = 0;
	handler->ccache = '\n';
	if (data_block_size == 0) {
		handler->DATA_BLOCK_SIZE = DEFAULT_DATA_BLOCK_SIZE;
	} else {
		handler->DATA_BLOCK_SIZE = data_block_size;
	}
	return handler;
}

/*
* mclose()
* @handler: handler of savadata
*/
void
mclose(MFILE *handler)
{
	DATA_BLOCK *n1;
	DATA_BLOCK *n2;

	if (NULL == handler)
		return;
	n1 = TAILQ_FIRST(&handler->head);
	while (n1 != NULL) {
		n2 = TAILQ_NEXT(n1, entries);
		handler->free_fun(n1->data);
		handler->free_fun(n1);
#ifdef MALLOC_DEBUG
		total_free += 2;
#endif
		n1 = n2;
	}
	free(handler);
#ifdef MALLOC_DEBUG
	++total_free;
#endif
}

/* for Read-Many */
void
mseek(MFILE *handler)
{
	DATA_BLOCK *n1;
	DATA_BLOCK *n2;

	if (NULL == handler)
		return;
	handler->is_empty = 0;
	handler->current_read_p = NULL;
	n1 = TAILQ_FIRST(&handler->head);
	while (n1 != NULL) {
		n1->current_read_count = 0;
		n2 = TAILQ_NEXT(n1, entries);
		n1 = n2;
	}
}

void mseek_pos(MFILE *handler, int pos)
{
	pos = pos < 0 ? 0 : pos;
	mseek( handler );
	DATA_BLOCK *n1;
	n1 = TAILQ_FIRST(&handler->head);
	while (n1 != NULL) {
		if ( pos <= n1->current_write_count ) {
			n1->current_read_count = pos;
			handler->current_read_p = n1;
			break;
		}
		pos -= n1->current_write_count;
		n1 = TAILQ_NEXT(n1, entries);
	}
}

/*
* mwrite()
* @handler: handler of savadata
* @data: the data you want save
* @len: length of data
* Upon successful completion mwrite() return 1.
* Otherwise, -1 is returned
*/
int
mwrite(MFILE *handler, const char *data, int len)
{
	DATA_BLOCK *new_block;
	DATA_BLOCK *current_block;
	int free_count;
	int writed;
	char *curr_p;

	if (NULL == handler)
		return -1;
	writed = 0;
	handler->total_size += len;
	while (writed < len) {
		if (handler->is_full) {
			new_block = handler->alloc_fun(sizeof(DATA_BLOCK));
			if (NULL == new_block) {
				fprintf(stderr, "alloc_fun data_block error\n");
				return -1;
			}
#ifdef MALLOC_DEBUG
++total_malloc;
#endif
			memset(new_block, 0, sizeof(DATA_BLOCK));
			new_block->data = (char *)handler->alloc_fun(handler->DATA_BLOCK_SIZE);
			if (NULL == new_block->data) {
				fprintf(stderr, "alloc_fun data buffer error\n");
				handler->free_fun(new_block);
				/*#ifdef MALLOC_DEBUG
				++total_free;
				#endif*/
				return -1;
			}
#ifdef MALLOC_DEBUG
++total_malloc;
#endif
			handler->block_num++;
			handler->current_write_p = new_block;
			TAILQ_INSERT_TAIL(&handler->head, new_block, entries);
		}
		current_block = handler->current_write_p;
		free_count = handler->DATA_BLOCK_SIZE - (current_block->current_write_count);
		curr_p = current_block->data
			+ (current_block->current_write_count);
		if ((len - writed) <= free_count) {
			memcpy(curr_p, data + writed, len - writed);
			current_block->current_write_count += (len - writed);
			current_block->data_len = current_block->current_write_count;
			if (current_block->current_write_count == handler->DATA_BLOCK_SIZE) {
				handler->is_full = 1;
			} else {
				handler->is_full = 0;
			}
			break;
		} else {
			memcpy(curr_p, data + writed, free_count);
			current_block->current_write_count += free_count;
			current_block->data_len = current_block->current_write_count;
			writed += free_count;
			handler->is_full = 1;
		}
	}
	return 1;
}

/*
* mwrite_head()
* @handler: handler of savadata
* @data: the data you want save
* @len: length of data
* Upon successful completion mwrite() return 1.
* Otherwise, -1 is returned
*/
int
mwrite_head(MFILE *handler, const char *data, int len)
{
	DATA_BLOCK *new_block;
	char *curr_p;

	if (NULL == handler)
		return -1;
	if (len > handler->DATA_BLOCK_SIZE) {
		fprintf(stderr, "mwrite_head data too long\n");
		return -1;
	}
	handler->total_size += len;
	new_block = handler->alloc_fun(sizeof(DATA_BLOCK));
	if (NULL == new_block) {
		fprintf(stderr, "alloc_fun data_block error\n");
		return -1;
	}
	memset(new_block, 0, sizeof(DATA_BLOCK));
	new_block->data = (char *)handler->alloc_fun(handler->DATA_BLOCK_SIZE);
	if (NULL == new_block->data) {
		fprintf(stderr, "alloc_fun data buffer error\n");
		handler->free_fun(new_block);
		return -1;
	}
	handler->new_header_size += len;
	handler->block_num++;
	TAILQ_INSERT_HEAD(&handler->head, new_block, entries);
	curr_p = new_block->data;
	new_block->current_write_count = len;
	new_block->data_len = len;
	memcpy(curr_p, data, len);
	return 1;
}

/*
* mread()
* @handler: handler of savadata
* @data: the data you want read
* @len: length of data buffer
* Upon successful completion mread() return size that you readed.
* in the end, -1 is returned
*/
int
mread(MFILE *handler, char *data, int len)
{
	DATA_BLOCK *new_block;
	DATA_BLOCK *current_block;
	int could_read;
	char *curr_p;

	if (NULL == handler)
		return -1;

	if (NULL == handler->current_read_p) {
		handler->current_read_p = TAILQ_FIRST(&handler->head);
		if (NULL == handler->current_read_p) {
			return -1;
		}
	}

	int readed = 0;
	while (readed < len) {
		if (handler->is_empty) {
			new_block = TAILQ_NEXT(handler->current_read_p, entries);
			if (NULL == new_block) {
				//if ( readed == 0 ) {
				//	return -1;
				//}
				//else {
				//	break;
				//}
				break;
			}
			handler->current_read_p = new_block;
		}

		current_block = handler->current_read_p;
		could_read = (current_block->current_write_count)
			- (current_block->current_read_count);
		if ( could_read <= 0 ) {
			break;
		}

		curr_p = current_block->data + (current_block->current_read_count);
		if ((len - readed) <= could_read) {
			memcpy(data + readed, curr_p, (len - readed));
			current_block->current_read_count += (len - readed);
			readed += (len - readed);
			if (current_block->current_read_count
				== current_block->current_write_count) {
					handler->is_empty = 1;
			}
			else {
				handler->is_empty = 0;
			}
			break;
		}
		else {
			memcpy(data + readed, curr_p, could_read);
			current_block->current_read_count += could_read;
			readed += could_read;
			handler->is_empty = 1;
		}
	}

	return readed;
}

/*
* mgetc()
* @handler: handler of savadata
* Upon successful completion mgetc() return char that you readed.
* in the end, 0 is returned
*/
char
mgetc(MFILE *handler)
{
	DATA_BLOCK *new_block;
	DATA_BLOCK *current_block;
	//int could_read;
	char *curr_p;

	if (NULL == handler)
		return 0;
	if (NULL == handler->current_read_p) {
		handler->current_read_p = TAILQ_FIRST(&handler->head);
		if (handler->current_read_p == NULL) {
			return 0;
		}
	}
	if (handler->is_empty) {
		//next:
		new_block = TAILQ_NEXT(handler->current_read_p, entries);
		if (NULL == new_block) {
			return 0;
		}
		handler->current_read_p = new_block;
	}
	current_block = handler->current_read_p;
	//could_read = (current_block->current_write_count)
	//- (current_block->current_read_count);
	curr_p = current_block->data
		+ (current_block->current_read_count);
	//if (could_read > 0) {
	//memcpy(data + readed, curr_p, (len - readed));
	current_block->current_read_count++;
	if (current_block->current_read_count
		== current_block->current_write_count) {
			handler->is_empty = 1;
	} else {
		handler->is_empty = 0;
	}
	return *curr_p;
	//} else {
	//    goto next;
	//}
}

/*
* mread_line()
* @handler: handler of savadata
* @buf: the data you want read
* @n: length of data buffer
* Upon successful completion mread_line() return size that you readed.
* in the end, 0 is returned
*/
int
mread_line(MFILE *handler, char *buf, int n)
{
	int r;

	if (NULL == handler)
		return 0;
	r = 0;
	buf[n-1] = '\0';
	n -= 2;
	if (n != 0) {
		char *d = buf;
		do {
			if ('\n' == handler->ccache) {
				*d = mgetc(handler);
			} else {
				*d = handler->ccache;
				handler->ccache = '\r';
			}
			if (*d == '\r') {
				handler->ccache = mgetc(handler);
				if ( '\0' == handler->ccache ) {
					*(++d) = '\n';
					*(++d) = '\0';
					r = d - buf;
					break;
				}
				else if ('\n' != handler->ccache) {
					d++;
					r = d - buf;
					break;
				}
				else {
					d++;
					*d = handler->ccache;
					d++;
					r = d - buf;
					break;
				}
			} else if (*d == '\n') {
				d++;
				r = d - buf;
				break;
			} else if (*d == 0) {
				r = d - buf;
				break;
			}
			d++;
		} while (--n > 0);
	}

	/* return the position that the string ends.*/
	return (r);
}

/*
* mwrite_file()
* @handler: handler of savadata
* Upon successful completion mwrite_file() the number of bytes
* which were written is returned.
* Otherwise, -1 is returned
*/
int
mwrite_file(MFILE *handler, int fd)
{
	const int MAX_BLOCK_NUMBER = 1024;
	struct iovec iov[MAX_BLOCK_NUMBER];
	int n_iov;
	DATA_BLOCK *dbnp;
	int file_size;
	int retval;

	if (NULL == handler) {
		return -1;
	}
	n_iov = 0;
	dbnp = NULL;
	file_size = 0;
	TAILQ_FOREACH(dbnp, &(handler->head), entries) {
		iov[n_iov].iov_base = dbnp->data;
		iov[n_iov].iov_len = dbnp->data_len < 0 ? 0 : dbnp->data_len;
		n_iov++;
		file_size += dbnp->data_len;
		if (n_iov >= MAX_BLOCK_NUMBER)
			return -1;
	}
	if (file_size == 0) {
		return 0;
	}
	retval = writev(fd, iov, n_iov);
	if (retval < 0) {
		return -1;
	} else {
		return retval;
	}
}

/*
* msize()
* @handler: handler of savadata
* Upon successful completion msize() the number of bytes
* which were written is returned.
* Otherwise, -1 is returned
*/
int
msize(MFILE *handler)
{
	return handler->total_size;
}


unsigned int mfile_new_header_size( MFILE *handler )
{
	return handler->new_header_size;
}

int mfile_copy( MFILE* dest, MFILE* src, unsigned int start, unsigned int end )
{
	char buffer[1024];
	mseek_pos( src, start );

	unsigned int i = start;
	unsigned int j = 0;
	int ret;
	while ( i < end ) {
		j = end - i;
		j = j<1024 ? j : 1024;
		ret = mread( src, buffer, j );
		if ( ret <= 0 ) {
			return -1;
		}
		else {
			mwrite( dest, buffer, ret );
			if ( ret < 1024 ) {
				break;
			}
		}
		i += j;
	}
	return end-start;
}

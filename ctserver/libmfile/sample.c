#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "mfile.h"

#define MAX_BUF_SIZE	1024
#define MAX_BLOCK_SIZE  8 * 1024


void usage(char *prog)
{
	printf("%s [file] [add header message]\n", prog);
}


int main(int argc, char **argv)
{

	if (argc != 3) {
		usage(argv[0]);
		return 0;
	}

	char *eml = argv[1];

	int fp = open(eml, O_RDONLY);
    if (fp == -1) {
        printf("open file: %s fail:%s\n", eml, strerror(errno));
        return 1;
    }   

    MFILE *mfp = mopen(MAX_BLOCK_SIZE, NULL, NULL);
    if (mfp == NULL) {
        printf("mopen fail\n");
        close(fp);

        return 1;
    }   

	char buf[MAX_BUF_SIZE] = {0};
	int n = 0;
    int mn = 0;
	while (1) {
		n = read(fp, buf, sizeof(buf)); 
		if (n == 0)
			break;

		if (n < 0) {
            close(fp);
            mclose(mfp);
            printf("read fail\n");

            return 1;
        }

		mn = mwrite(mfp, buf, n); 
        if (mn != 1) {
            close(fp);
            mclose(mfp);
            printf("mwrite fail\n");

            return 1;
        }	
	}

	// 加头 
	char add_header[1024] = {0};
    int add_header_len = strlen(argv[2]);
    memcpy(add_header, argv[2], add_header_len);
    add_header[add_header_len] = '\r';
    add_header[add_header_len+1] = '\n';

    mn = mwrite_head(mfp, add_header, add_header_len + 2);
    if (mn != 1) {
        printf("mwrite_head fail\n");
        mclose(mfp);
        return 1;
    }


	while (1) {
		memset(buf, 0, sizeof(buf));
        n = mread_line(mfp, buf, sizeof(buf));
        if (n == 0)
            break;

        printf("%s\n", buf);
	}

	printf("################# Start Output ################\n");
	mn = mwrite_file(mfp, 1);
	printf("################# END Output ##################\n");
    printf("mwrite_file len:%d\n", mn);

    mclose(mfp);


	return 0;
}


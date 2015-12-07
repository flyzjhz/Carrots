//
//  ctmaster.c
//  
//
//  Created by SongJian on 15/12/6.
//
//

#include "ctmaster.h"

#include "confparser.h"
#include "dictionary.h"

#include "ctlog.h"
#include "ctutils.h"

#define DEF_LOGLEVEL            "7"
#define DEF_MAXCHILDS           "128"
#define DEF_OFFSETPATH          "./offset"
#define DEF_CHDIRPATH           "./"
#define DEF_WAITSLEEP           "5"
#define DEF_EXITTIME			"86400"


// save config from read config file
dictionary *dict_conf          = NULL;

struct config_st config_t;
struct childs_st *childs_t     = NULL;


dictionary *open_config(char *config_file)
{
    dictionary *dict = open_conf_file(config_file);
    if (dict == NULL) {
        log_error("open config file fail:%s", config_file);
        return NULL;
    }
    
    return dict;
}

void close_config(dictionary *dict)
{
    if (dict != NULL) {
        dictionary_del(dict);
        dict = NULL;
    }
}

int read_config(dictionary *dict)
{
    if (dict == NULL) {
        printf("dictionary is NULL, please call open_config first\n");
        return 1;
    }
    
    // log leve
    char *plog_level = dictionary_get(dict, "global:log_level", NULL);
    if (plog_level == NULL) {
        log_warning("parse config 'log_level' fail, use default:%s", DEF_LOGLEVEL);
        snprintf(config_t.log_level, sizeof(config_t.log_level), "%s", DEF_LOGLEVEL);
    } else {
        snprintf(config_t.log_level, sizeof(config_t.log_level), "%s", plog_level);
    }
    
    // process file
    char *pprocess_file = dictionary_get(dict, "global:process_file", NULL);
    if (pprocess_file == NULL) {
        printf("no file was process");
        return 1;
    } else {
        snprintf(config_t.process_file, sizeof(config_t.process_file), "%s", pprocess_file);
    }
    
    // chdir path
    char *pchdir_path = dictionary_get(dict, "global:chdir_path", NULL);
    if (pchdir_path == NULL) {
        log_warning("parse config 'chdir_path' fail, use default:%s", DEF_CHDIRPATH);
        snprintf(config_t.chdir_path, sizeof(config_t.chdir_path), "%s", DEF_CHDIRPATH);
    } else {
        snprintf(config_t.chdir_path, sizeof(config_t.chdir_path), "%s", pchdir_path);
    }
    
    // offset path
    char *poffset_path = dictionary_get(dict, "global:offset_path", NULL);
    if (poffset_path == NULL) {
        log_warning("parse config 'offset_path' fail, use default:%s", DEF_OFFSETPATH);
        snprintf(config_t.offset_path, sizeof(config_t.offset_path), "%s", DEF_OFFSETPATH);
    } else {
        snprintf(config_t.offset_path, sizeof(config_t.offset_path), "%s", poffset_path);
    }
    
 	// wait sleep 
    char *pwait_sleep = dictionary_get(dict, "global:wait_sleep", NULL);
    if (pwait_sleep == NULL) {
        log_warning("parse config 'wait_sleep' fail, use default:%s", DEF_WAITSLEEP);
        snprintf(config_t.wait_sleep, sizeof(config_t.wait_sleep), "%s", DEF_WAITSLEEP);
    } else {
        snprintf(config_t.wait_sleep, sizeof(config_t.wait_sleep), "%s", pwait_sleep);
    }
    
    // exit time
    char *pexit_time = dictionary_get(dict, "global:exit_time", NULL);
    if (pexit_time == NULL) {
        log_warning("parse config 'exit_time' fail, use default:%s", DEF_EXITTIME);
        snprintf(config_t.exit_time, sizeof(config_t.exit_time), "%s", DEF_EXITTIME);
    } else {
        snprintf(config_t.exit_time, sizeof(config_t.exit_time), "%s", pexit_time);
    }

    // max child number
    char *pmax_childs = dictionary_get(dict, "global:max_childs", NULL);
    if (pmax_childs == NULL) {
        log_warning("parse config 'max_childs' fail, use default:%s", DEF_MAXCHILDS);
        snprintf(config_t.max_childs, sizeof(config_t.max_childs), "%s", DEF_MAXCHILDS);
    } else {
        snprintf(config_t.max_childs, sizeof(config_t.max_childs), "%s", pmax_childs);
    }
    
    log_debug("parse config file succ");
    return 0;
    
}

/**
 *  获取一个空闲的索引
 *
 *  @return 返回索引，－1: 失败
 */
int get_idle_idx_from_childs_t()
{
    int idx = -1;
    int i = 0;
    for (i = 0; i<(atoi(config_t.max_childs) + 1); i++) {
        if (childs_t[i].used == 0) {
            idx = i;
            break;
        }
    }
    
    return idx;
}

void init_child_with_idx(int i)
{
    childs_t[i].used = 0;
    childs_t[i].pid = -1;
}

void clean_child_with_idx(int i)
{
    childs_t[i].used = 0;
    childs_t[i].pid = -1;
}


void sigchld_exit()
{
    pid_t pid;
    int wstat;
    while ((pid = waitpid(-1, &wstat, WNOHANG)) > 0) {
        int i = 0;
        while (i < atoi(config_t.max_childs) + 1) {
            if (childs_t[i].pid == pid) {
                clean_child_with_idx(i);
                break;
            }
            
            i++;
        }
    }
}



int init_for_rindex(struct file_info_t *file_info_st, char *offset_file)
{
	struct stat st;

    file_info_st->file_rindex_fd = 0;
    file_info_st->file_rindex_offset_ptr = NULL;
    file_info_st->rindex_offset = 0;
    
    file_info_st->file_rindex_fd = open(offset_file, O_RDWR| O_NDELAY | O_CREAT, 0600);
    if (file_info_st->file_rindex_fd == -1) {
        printf("open file:%s fail:%m\n", offset_file);
        log_error("open file:%s fail:%m", offset_file);
        return 1;
    }
    
    fcntl(file_info_st->file_rindex_fd, F_SETFD, 1);	// 禁止与子进程共享
    
    if (fstat(file_info_st->file_rindex_fd, &st) == -1) {
        printf("fstat file:%s fail:%m\n", offset_file);
        log_error("fstat file:%s fail:%m", offset_file);
        close(file_info_st->file_rindex_fd);
        return 1;
    }
    
    if (st.st_size < 1) {
        ftruncate(file_info_st->file_rindex_fd, 1024);
    }
    
    file_info_st->file_rindex_offset_ptr = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, file_info_st->file_rindex_fd, 0);
    if (file_info_st->file_rindex_offset_ptr == MAP_FAILED) {
        printf("mmap file:%s fail:%m\n", offset_file);
        log_error("mmap file:%s fail:%m", offset_file);
        close(file_info_st->file_rindex_fd);
        return 1;
    }
    
    if (st.st_size > 0) {
        file_info_st->rindex_offset = atoll(file_info_st->file_rindex_offset_ptr);
    }
    
    return 0;
}


void process_via_child(char *buf, FILE *fp, long offset)
{
    int i = -1;
    while (1) {
        i = get_idle_idx_from_childs_t();
        if (i < 0) {
            sleep(atoi(config_t.wait_sleep));
        }
        break;
    }
    
    pid_t pid;
    while ((pid = fork()) == -1) {
        log_error("fork fail:%m");
        sleep(atoi(config_t.wait_sleep));
    }
    
    if (pid > 0) {
        childs_t[i].used = 1;
        childs_t[i].pid = pid;
        
    } else if (pid == 0) {
        fclose(fp);
        fp = NULL;
        
        // do something ...
        // int succ = child_process_file(config_t.process_file, offset, buf);
        
        exit(0);
    }
}



void usage(char *arog)
{
    printf("%s -c[config file] [-d]\n");
    printf("-d:     to be a daemon");
}

int main(int argc, char **argv)
{
    if (argc != 3 && argc != 4) {
        usage(argv[0]);
        exit(0);
    }
    
    char cfg_file[MAX_LINE] = {0};
    int make_deamon = 0;
	unsigned int exit_time = 0;
    
    int ch;
    const char *args = "c:dh";
    while ((ch = getopt(argc, argv, args)) != -1) {
        switch (ch) {
            case 'c':
                snprintf(cfg_file, sizeof(cfg_file), "%s", optarg);
                break;
            case 'd':
                make_deamon = 1;
                break;
                
            case 'h':
            default:
                usage(argv[0]);
                exit(0);
                break;
        }
    }
    
    if (make_deamon == 1) {
        // create deamon
        create_daemon(config_t.chdir_path);
    }
    
    // init log
    ctlog(argv[0], LOG_PID|LOG_NDELAY, LOG_MAIL);
    
    // read config
    dict_conf = open_config(cfg_file);
    if (dict_conf == NULL) {
        printf("parse config fail");
        return 1;
    }
    if (read_config(dict_conf) != 0) {
        return 1;
    }
    log_level = atoi(config_t.log_level);
    
    if (chdir(config_t.chdir_path) == -1) {
        log_error("can not start: unable to change directory:%s", config_t.chdir_path);
        return 1;
    }
    log_debug("log_level:%s", config_t.log_level);
    log_debug("chdir_path:%s", config_t.chdir_path);
    log_debug("offset_path:%s", config_t.offset_path);
    log_debug("max_childs:%s", config_t.max_childs);
    
    // ---------- ----------
    
    childs_t = (struct childs_st *)malloc((atoi(config_t.max_childs) + 1) * sizeof(struct childs_st));
    if (childs_t == NULL) {
        log_error("malloc childs [%d] faild:[%d]:%s", (atoi(config_t.max_childs) + 1), errno, strerror(errno));
        exit(1);
    }
    
    int i = 0;
    for (i=0; i<(atoi(config_t.max_childs) +1); i++) {
        init_child_with_idx(i);
    }
    
    // 捕抓子进程退出信号
    sig_catch(SIGCHLD, sigchld_exit);
    
    
    // ------ 开始 ------
    // 初始化，读取文件索引内容
    struct tm *tmp;
    struct stat st;
    
    char now_str[1024] = {0};
    time_t cur = time(NULL);
    
    tmp = localtime(&cur);
    strftime(now_str, sizeof(now_str) - 1, "%Y%m%d", tmp);
    
    if (*(config_t.offset_path) == '\0') {
        config_t.offset_path[0] = '.';
        config_t.offset_path[1] = '\0';
    } else {
        if ( is_dir_exits(config_t.offset_path) != 0 ) {
            log_error("is_dir_exit fail");
            return 1;
        }
    }
    
    char offset_file[MAX_LINE] = {0};
    snprintf(offset_file, sizeof(offset_file), "%s/%s.%s", config_t.offset_path, OFFSET_FILENAME, now_str);
    
    struct file_info_t file_info_st;
    if (init_for_rindex(&file_info_st, offset_file) != 0) {
        printf("init_for_rindex fail\n");
        log_error("init_for_rindex fail");
        return 1;
    }
    
    
    // 打开被分析的文件
    char buf[BUF_SIZE] = {0};
    FILE *fp = fopen(config_t.process_file, "r");
    if (!fp) {
        printf("open file:%s fail:%m\n", config_t.process_file);
        log_error("open file:%s fail:%m", config_t.process_file);
        if (file_info_st.file_rindex_fd)
            close(file_info_st.file_rindex_fd);
        return 1;
    }
    
    // 如果偏移量大于0，则偏移文件到上次读取的地方
    if (file_info_st.rindex_offset > 0) {
        if (lseek(fileno(fp), file_info_st.rindex_offset, SEEK_SET) == -1) {
            printf("lseek fail:%m\n");
            log_error("lseek fail:%m");
            fclose(fp);
            if (file_info_st.file_rindex_fd)
                close(file_info_st.file_rindex_fd);
            return 1;
        }
    }
    
    // 循环读取文件，读一行分析,如果是逻辑开始行, 创建一个子进程去执行
    while (1) {
        if (!fgets(buf, sizeof(buf)-1, fp)) {
            // 读取失败
            if (is_need_master_exit(fileno(fp), config_t.process_file, atoi(config_t.exit_time)) == 1) {
                log_info("file:%s was expire, bye!", config_t.process_file);
                goto MASTER_BYE;
            }
            
            log_debug("fgets is null, sleep 5 second");
            sleep(atoi(config_t.wait_sleep));
            
            continue;
        }
        
        if (*buf == '\0') {
            continue;
        }
        
        
        if ( strstr(buf, " qmail-smtpd[")
            && strstr(buf, "]: [INFO] mail_process ICID:")
            && strstr(buf, " LOGExe:[-M") ) {
        
            // 保存当前读取的偏移量
            file_info_st.rindex_offset = ftell(fp);
            if (file_info_st.rindex_offset > 0) {
                snprintf(file_info_st.file_rindex_offset_ptr, 1023, "%ld", file_info_st.rindex_offset);
            }
            
            
            // 处理
            process_via_child(buf, fp, file_info_st.rindex_offset);
        }
    }
    
MASTER_BYE:
    fclose(fp);
    
    if (file_info_st.file_rindex_fd)
        close(file_info_st.file_rindex_fd);
    
    if (childs_t != NULL) {
        free(childs_t);
        childs_t = NULL;
    }
    
    return 0;
    
}




/*
 * OS Assignment #4
 *
 * @file pfs.c
 */

#include <fuse.h>
#include <errno.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dirent.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <signal.h>

/* util */
void get_command_line(int pid, char * out)
{
    int i = 0;
    char path[256];
    FILE* fp;
    sprintf(path, "/proc/%d/cmdline", pid);

    fp = fopen(path, "r");
    if(!fp) return;

    fgets(out, 256, fp);

    if(out[0] == '/')
        strcpy(out, out + 1);

    for( i = 0; i < strlen(out); i++)
    {
        if(out[i] == '/') out[i] = '-';
    }

    fclose(fp);
}

int get_vmsize(int pid)
{
    char path[256];
    char line[256];
    FILE* fp;
    int size;

    sprintf(path, "/proc/%d/status", pid);

    fp = fopen(path, "r");
    if(!fp) return 0;

    while(fgets(line, 256, fp) != NULL)
    {
        if(strstr(line, "VmSize:"))
        {
            sscanf(line, "VmSize:%d", &size);
            break;
        }
    }

    fclose(fp);

    return size;
}

typedef struct _process_data{
    int         _pid;

    char        _name[256];
    int         vm_size;
    struct stat _stat;
} process_data;

process_data*   proc_data[32768];
int             n_proc_data = 0;

process_data*   find_process_data(int pid)
{
    int i=0;
    for(i = 0; i < n_proc_data; i++)
    {
        if(pid == proc_data[i]->_pid)
        {
            return proc_data[i];
        }
    }
    return NULL;
}

void release_all_process_data()
{
    int i=0;
    for(i = 0; i < n_proc_data; i++)
    {
        if(proc_data[i] != 0) {
            free(proc_data[i]);
            proc_data[i] = 0;
        }
    }
    n_proc_data = 0;
}

void update_all_process_data()
{
    DIR* dir = NULL;
    struct dirent* entry;	// DIRectory ENTry
    dir = opendir("/proc");

    release_all_process_data();

    while( (entry = readdir(dir)) != NULL ) {

        char 		path[256];

        int         pid = -1;
        int         vm_size = 0;
        char        cmdline[256];
        struct stat fileStat;

        sprintf(path, "/proc/%s", entry->d_name);

        lstat(path, &fileStat);
        if(!S_ISDIR(fileStat.st_mode))
            continue;

        pid = atoi(entry->d_name);
        if( pid <= 0 )
            continue;

        get_command_line(pid, cmdline);
        vm_size = get_vmsize(pid);

        if(proc_data[n_proc_data] == 0)
            proc_data[n_proc_data] = (process_data*)malloc(sizeof(process_data));

        proc_data[n_proc_data]->_pid    = pid;
        proc_data[n_proc_data]->vm_size = vm_size;
        proc_data[n_proc_data]->_stat   = fileStat;
        sprintf(proc_data[n_proc_data]->_name, "%d-%s", pid, cmdline);

        n_proc_data++;
    }
}

void print_all_process_info()
{
    int i=0;
    for(i = 0; i < n_proc_data; i++)
    {
        process_data* dat = proc_data[i];
        printf("[%d] %s\n", dat->_pid, dat->_name);
    }
}

static int pfs_getattr(const char  *path, struct stat *stbuf)
{
    int res = 0;

    memset(stbuf, 0, sizeof(struct stat));

    if(strcmp(path, "/") == 0) {

        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else {

        int i = 0;
        for (i = 0; i < n_proc_data; i++)
        {
            if (strcmp(path + 1, proc_data[i]->_name) == 0)
            {
                stbuf->st_mode = S_IFREG | 0644;
                stbuf->st_nlink = 1;
                stbuf->st_size = proc_data[i]->vm_size;

                return res;
            }
        }

        return -ENOENT;
    }

    return res;
}

static int pfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    int res = 0;
    int i = 0;

    (void) offset;
    (void) fi;

    if(strcmp(path, "/") != 0)
        return -ENOENT;

    // update process data
    update_all_process_data();

    // and filter others
    for(i = 0; i < n_proc_data; i++)
    {
        filler(buf, proc_data[i]->_name, NULL, 0);
    }

    return res;
}

// deleting file
static int pfs_unlink(const char *path)
{
    int res = 0;
    int pid = -1;

    sscanf(path+1, "%d", &pid);
    printf("UNLINK PATH IS %s pid is %d\n", path, pid);

    if(pid <= 0)
        return -ENOENT;

    kill(pid, SIGKILL);

    return res;
}

static struct fuse_operations pfs_oper =
{
    .getattr  = pfs_getattr,
    .readdir  = pfs_readdir,
    .unlink   = pfs_unlink
};


int main (int argc, char **argv)
{
    int ret = 0;
    update_all_process_data();

    ret = fuse_main (argc, argv, &pfs_oper);
    release_all_process_data();
    return ret;
}

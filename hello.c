/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2005  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello";

struct stat t;
struct timespec spec;

// getting files' attributes
static int hello_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;

    printf("hello get attr : [%s]\n",path);

    memset(stbuf, 0, sizeof(struct stat));  // clear 'stbuf' ( which is out parameter )

    if(strcmp(path, "/") == 0) {            // if 'path' is root
        stbuf->st_mode = S_IFDIR | 0755;    // directory 0755
        stbuf->st_nlink = 2;                // link count is 2
    }
    else if(strcmp(path, hello_path) == 0) {
        stbuf->st_mode = S_IFREG | 0444;    // regular file 0444
        stbuf->st_nlink = 1;                // link count is 1
        stbuf->st_size = strlen(hello_str); // setting file's size
    }
    else
        res = -ENOENT;                      // no such a file or dir

    return res;
}

// read directory
static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *fi)
{
    // Why casting with void?
    // it is there to avoid warnings from the compiler because some parameters are unused.
    (void) offset;
    (void) fi;

    if(strcmp(path, "/") != 0)              // if path is not root ( we don't need others )
        return -ENOENT;                     // return error

    filler(buf, ".", NULL, 0);              // what is the filter function?
    filler(buf, "..", NULL, 0);             // create?
    filler(buf, hello_path + 1, NULL, 0);

    return 0;
}

// file open
static int hello_open(const char *path, struct fuse_file_info *fi)
{
    if(strcmp(path, hello_path) != 0)       // if not hello path
        return -ENOENT;                     // error

    if((fi->flags & 3) != O_RDONLY)         // if file is not readonly
        return -EACCES;                     // error

    return 0;
}

// file read
static int hello_read(const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi)
{
    size_t len;
    (void) fi;                              // useless void casting

    if(strcmp(path, hello_path) != 0)
        return -ENOENT;

    len = strlen(hello_str);                // store string length of "Hello World"
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, hello_str + offset, size);
    } else
        size = 0;

    // Read should return exactly the number of bytes requested except on EOF or error,
    // otherwise the rest of the data will be substituted with zeroes.
    return size;
}

// setting operation callback
static struct fuse_operations hello_oper =
        {
                .getattr = hello_getattr,
                .readdir = hello_readdir,
                .open    = hello_open,
                .read    = hello_read,
        };

int main(int argc, char *argv[])
{
    // and execute fuse with designated functions.
    return fuse_main(argc, argv, &hello_oper);
}

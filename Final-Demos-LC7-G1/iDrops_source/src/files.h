#ifndef FILES
#define FILES
#include <dirent.h> 
#include <stdio.h> 
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <assert.h>

#define FILENAME "idrops.idc"
#define EXTENSION ".idc"
#define MAX_SETS 4
#define KEYVALMAX 5
#define BUFSIZE 256
#define DEFAULT_FP_LEN 5
#define DEFAULT_DIR_LEN 5
#define CFG_ALL (-1)

enum cfg_state{CFG_OK, CFG_TOO_BIG, CFG_MALFORMED, CFG_MALLOC_FAIL};
enum dir_state{DIR_OK, DIR_OPEN_FAIL, DIR_MALLOC_FAIL};

struct file_pair {
  char **files;
  char **names;
};

struct set {
  char *setname;
  struct file_pair fp;
};

struct config_info {
  struct file_pair fp;
  size_t length;
  int status;
};

struct dir_list {
  char **list;
  size_t length;
  int status;
};

struct config_info parsecfg(FILE *, int);
struct dir_list getconfigs(void);


/* takes blank config_info */
struct config_info parsecfg(FILE *cfg, int max_count)
{
  assert(DEFAULT_FP_LEN > 0);

  char *n, *m;
  char buf[BUFSIZE];
  int line = 1;
  size_t loc = 0;
  struct config_info info;
  info.fp.files = malloc(sizeof(char**) * DEFAULT_FP_LEN);
  info.fp.names = malloc(sizeof(char**) * DEFAULT_FP_LEN);
  
  if (info.fp.names == NULL || info.fp.files == NULL) {
    fprintf(stderr, "Failed to allocate memory for config_info\n");
    info.status = CFG_MALLOC_FAIL;
    return info;
  }
  
  info.length = DEFAULT_FP_LEN;
  
  while( fgets(buf, BUFSIZE, cfg) != NULL) {
    n = buf;
    int blank = 1;
    int comment = 0;
    while (*n != ':' && *n) {
      if (*n == '#' && blank)  {
        comment = 1;
        break;
      }
      if (*n != '\t' && *n != ' ' && *n != '\n')
        blank = 0;
      n++;
    }
    
    if (!blank || !comment) {
      if (*n == '\0') {
        fprintf(stderr, "missing ':' on line %d.\n", line);
        info.status = CFG_MALFORMED;
        return info;
      }
      
      *n = '\0';
      
      n++;
      
      while (*n != '"' && *n && *n != '\n') /* FIXME: this will not allow filenames with quotes */
        n++;
      
      if (*n == '\n' || *n == '\0') {
        fprintf(stderr, "Missing \" on line %d", line);
        info.status = CFG_MALFORMED;
        return info;
      }
      
      m = n + 1;
      
      while(*m != '"' && *m != '\n' && *m) {
        m++;
      }
      
      *m = '\0';
      
      if (loc >= max_count && max_count != CFG_ALL) {
        /* return status indicating there are still options left */
        info.status = CFG_TOO_BIG;
        return info;
      }
      if (loc >= info.length) {
        info.fp.files = realloc(info.fp.files, sizeof(char**) * (loc + 1));
        info.fp.names = realloc(info.fp.names, sizeof(char**) * (loc + 1));

        if (info.fp.names == NULL || info.fp.files == NULL)  {
          fprintf(stderr, "Failed to allocate memory for config_info\n");
          info.status = CFG_MALLOC_FAIL;
          return info;
        }

        info.length++;
      }
      //printf("Key: %s ,Value: %s\n", buf, n + 1);
      info.fp.files[loc] = malloc(strlen(buf) * sizeof(char));
      info.fp.names[loc] = malloc(strlen(n+1) * sizeof(char));
      
      strcpy(info.fp.files[loc], buf);
      strcpy(info.fp.names[loc], n + 1);
      
      if (info.fp.files == NULL || info.fp.names == NULL) {
        fprintf(stderr, "Failed to allocate memory for config_info\n");
        info.status = CFG_MALLOC_FAIL;
        return info;
      }
      
      loc++;
    }
    line++;
  }
  info.status = CFG_OK;
  return info;
}

struct dir_list getconfigs()
{
  assert(DEFAULT_DIR_LEN > 0);

  struct dir_list dl;
  dl.length = 0;
  dl.list = malloc(sizeof(char**) * DEFAULT_DIR_LEN);
  for (int i = 0; i < DEFAULT_DIR_LEN; i++)
    dl.list[i] = NULL;


  size_t max_length = DEFAULT_DIR_LEN;
  struct dirent *dir;

  if (dl.list == NULL) {
    fprintf(stderr, "Failed to allocate memory for directory listings.\n");
    dl.status = DIR_MALLOC_FAIL;
    return dl;
  }
  
  DIR *dp = opendir("./");
  
  if (dp == NULL) {
    dl.status = DIR_OPEN_FAIL;
    return dl;
  }

  while ((dir = readdir(dp))) {
    if (dir->d_type == DT_REG) {
      char *b = dir->d_name;
      char *e = dir->d_name;
      
      while(*e)
        e++;
      e--;
      
      int i = 0;

      /* @HACK: fix comparison of long and unsiged long */
      if (e - b >= strlen(EXTENSION)) {
        for (i = strlen(EXTENSION) - 1; i >= 0; i--, e--) {
          if (EXTENSION[i] != *e)
            break;
        }
      }
      if (i == -1) {
        dl.list[dl.length] = malloc(sizeof(char) * strlen(dir->d_name));
        if (dl.list[dl.length] == NULL) {
          fprintf(stderr, "Failed to allocate memory for directory listings.\n");
          dl.status = DIR_MALLOC_FAIL;
          return dl;
        }

        strcpy(dl.list[dl.length], dir->d_name);
        dl.length++;

        if (dl.length == max_length) {
          max_length++;
          dl.list = realloc(dl.list, sizeof(char**) * max_length);
        }
      }
    }
  }
  
  closedir(dp);

  dl.status = DIR_OK;

  return dl;

}
#endif //FILES

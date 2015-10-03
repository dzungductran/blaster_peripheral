/*
Copyright © 2015 NoTalentHack, LLC

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "common.h"

#define MAXBUF 	1024
static char buffer[MAXBUF];

int filecopy(char *source_file, char *target_file)
{
   int status = 0, nread;
   char buf[1024];
   FILE *source, *target;
 
   source = fopen(source_file, "r");
   if( source == NULL )
   {
      sprintf(buffer, "Can't open file %s; missing file?", source_file);
      return -1;
   }
 
   target = fopen(target_file, "w");
   if( target == NULL )
   {
      fclose(source);
      sprintf(buffer, "Can't open file %s; need root permission", target_file);
      return -1;
   }

   while ((nread = fread(buf, 1, sizeof(buf), source)) > 0) {
      fwrite(buf, 1, nread, target);
      if (ferror(source)) {
         sprintf(buffer, "Error reading file %s", source_file);  
         status = -1;
         goto _donefilecopy;
      }
   }

_donefilecopy: 
   fclose(source);
   fclose(target);
 
   return status;
}

/*
 * Pointer to array allocated at run-time.
 */
static pid_t    *childpid = NULL;

/*
 * From our open_max(), {Prog openmax}.
 */
static int      maxfd;

FILE *mypopen(const char *cmdstring, const char *type)
{
    int     i;
    int     pfd[2];
    pid_t   pid;
    FILE    *fp;

    /* only allow "r" "e" or "w" */
    if ((type[0] != 'r' && type[0] != 'w' && type[0] != 'e') || type[1] != 0) {
        errno = EINVAL;     /* required by POSIX */
        return(NULL);
    }

    if (childpid == NULL) {     /* first time through */
        /* allocate zeroed out array for child pids */
        maxfd = 256;
        if ((childpid = calloc(maxfd, sizeof(pid_t))) == NULL)
            return(NULL);
    }

    if (pipe(pfd) < 0)
        return(NULL);   /* errno set by pipe() */

    if ((pid = fork()) < 0) {
        return(NULL);   /* errno set by fork() */
    } else if (pid == 0) {                          /* child */
        if (*type == 'e') {
            close(pfd[0]);
            if (pfd[1] != STDERR_FILENO) {
                dup2(pfd[1], STDERR_FILENO);
                close(pfd[1]);
            }
        } else if (*type == 'r') {
            close(pfd[0]);
            if (pfd[1] != STDOUT_FILENO) {
                dup2(pfd[1], STDOUT_FILENO);
                close(pfd[1]);
            }
        } else {
            close(pfd[1]);
            if (pfd[0] != STDIN_FILENO) {
                dup2(pfd[0], STDIN_FILENO);
                close(pfd[0]);
            }
        }

        /* close all descriptors in childpid[] */
        for (i = 0; i < maxfd; i++)
            if (childpid[i] > 0)
                close(i);

        execl("/bin/sh", "sh", "-c", cmdstring, (char *)0);
        _exit(127);
    }

    /* parent continues... */
    if (*type == 'e') {
        close(pfd[1]);
        if ((fp = fdopen(pfd[0], "r")) == NULL)
            return(NULL);
    } else if (*type == 'r') {
        close(pfd[1]);
        if ((fp = fdopen(pfd[0], type)) == NULL)
            return(NULL);

    } else {
        close(pfd[0]);
        if ((fp = fdopen(pfd[1], type)) == NULL)
            return(NULL);
    }

    childpid[fileno(fp)] = pid; /* remember child pid for this fd */
    return(fp);
}

int mypclose(FILE *fp)
{
    int     fd, stat;
    pid_t   pid;

    if (childpid == NULL) {
        errno = EINVAL;
        return(-1);     /* popen() has never been called */
    }

    fd = fileno(fp);
    if ((pid = childpid[fd]) == 0) {
        errno = EINVAL;
        return(-1);     /* fp wasn't opened by popen() */
    }

    childpid[fd] = 0;
    if (fclose(fp) == EOF)
        return(-1);

    while (waitpid(pid, &stat, 0) < 0)
        if (errno != EINTR)
            return(-1); /* error other than EINTR from waitpid() */

    return(stat);   /* return child's termination status */
}

// execute command
int shellcmd(char *cmd, char *type){
    FILE *fp;
    char line[256];
    fp = mypopen(cmd, type);
    if (fp==NULL) return -1;

    memset(buffer, 0, MAXBUF);
    int pos = 0;
    /* Read the output a line at a time - output it. */
    while (fgets(line, sizeof(line)-1, fp) != NULL) {
        int len = strlen(line);
        if ((pos + len) > MAXBUF-1)
            break;      // too much for buffer
        strcpy(&buffer[pos], line);
        pos += len;
    }

    return WEXITSTATUS(mypclose(fp));
}

// copy error and return
int get_lasterror(char *error) {
    int len = strlen(buffer);
    memcpy(error, buffer, len);
    return len;
}

// return last error buffer
char *get_message() {
    return buffer;
}

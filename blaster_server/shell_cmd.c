/*
Copyright © 2015-2016 Dzung Tran (dzungductran@yahoo.com)

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

static char buffer[BUFSIZE];

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

// another exec
int shellcmd(char *cmdstring, char *type) {
    int pipefd[2];
    pipe(pipefd);
    int     stat;
    pid_t   pid;

    /* only allow "r" "e" or "o" */
    if ((type[0] != 'r' && type[0] != 'o' && type[0] != 'e') || type[1] != 0) {
        errno = EINVAL;     /* required by POSIX */
        return(1);
    }

    if ((pid = fork()) == 0)
    {
        close(pipefd[0]);    // close reading end in the child

        if (*type == 'r') {
           dup2(pipefd[1], 1);  // send stdout to the pipe
           dup2(pipefd[1], 2);  // send stderr to the pipe
        } else if (*type == 'o') {
           dup2(pipefd[1], 1);  // send stdout to the pipe
        } else {
           dup2(pipefd[1], 2);  // send stderr to the pipe
        }

        close(pipefd[1]);    // this descriptor is no longer needed

        execl("/bin/sh", "sh", "-c", cmdstring, (char *)0);
        _exit(127);
    }
    else
    {
        // parent
        close(pipefd[1]);  // close the write end of the pipe in the parent

	// always make sure last char is '\0', so read one less
    	memset(buffer, 0, BUFSIZE);
        int len = read(pipefd[0], buffer, sizeof(buffer)-1);
        if (len < 0) {
            fprintf(stderr, "Error reading from shell command %s %s\n", cmdstring, type);
        }
        printf(buffer);    

        close(pipefd[0]);
    }

    while (waitpid(pid, &stat, 0) < 0)
        if (errno != EINTR)
            return(-1); /* error other than EINTR from waitpid() */

    return WEXITSTATUS(stat);   /* return child's termination status */
}

// copy error and return
void get_lasterror(char *error) {
    int len = strlen(buffer);
    memcpy(error, buffer, len);
}

// return last error buffer
char *get_message() {
    return buffer;
}

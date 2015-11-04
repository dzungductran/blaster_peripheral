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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/times.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <malloc.h>  
#include<pthread.h>

#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#define BUFSIZE 256

struct pstat {
    char state;
    long unsigned int utime_ticks;
    long int cutime_ticks;
    long unsigned int stime_ticks;
    long int cstime_ticks;                            
    long unsigned int vsize; // virtual memory size in bytes
    long unsigned int rss; //Resident  Set  Size in bytes
    long unsigned int cpu_total_time;
};                                                   

/*
vendor_id       : GenuineIntel
cpu family      : 6
model           : 74
model name      : Genuine Intel(R) CPU   4000  @  500MHz
stepping        : 8
cpu MHz         : 500.000
cache size      : 1024 KB
cpu cores       : 2
*/
struct cpuInfo {
    int cpu_cores;
    float frequency;
    int cpu_family;
    int stepping;
    int model;
    char cache_size[32];
    char model_name[128];
    char vendor_id[32];
} ;


extern char *get_message();
extern void getCpuInfo(struct cpuInfo *cpuinfo);
extern void get_lasterror( char *msg );
extern int shellcmd(char *cmd, char *type);
extern int filecopy(char *src, char *target);
extern pid_t findCommand(const char *cmd);
extern int get_usage(const pid_t pid, struct pstat* result);
extern void calc_cpu_usage_pct2(const struct pstat* cur_usage,                                                                   
                        const struct pstat* last_usage,                                                                  
                        double* usage);
extern void calc_cpu_usage_pct(const struct pstat* cur_usage,                                                                   
                        const struct pstat* last_usage,                                                                  
                        double* usage);


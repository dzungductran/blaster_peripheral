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

/* Read process stat from
 * http://man7.org/linux/man-pages/man5/proc.5.html
 */
int get_usage(const pid_t pid, struct pstat* result) {

    int i;
    //convert  pid to string
    char pid_s[20];
    snprintf(pid_s, sizeof(pid_s), "%d", pid);

    char stat_filepath[30] = "/proc/"; strncat(stat_filepath, pid_s,
            sizeof(stat_filepath) - strlen(stat_filepath) -1);
    strncat(stat_filepath, "/stat", sizeof(stat_filepath) -
            strlen(stat_filepath) -1);

    FILE *fpstat = fopen(stat_filepath, "r");
    if (fpstat == NULL) {
        perror("FOPEN ERROR ");
        return -1;
    }

    FILE *fstat = fopen("/proc/stat", "r");
    if (fstat == NULL) {
        perror("FOPEN ERROR ");
        fclose(fstat);
        return -1;
    }

    //read values from /proc/pid/stat
    bzero(result, sizeof(struct pstat));
    long int rss;
    if (fscanf(fpstat, "%*d %*s %c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu"
                "%lu %ld %ld %*d %*d %*d %*d %*u %lu %ld",
                &result->state, &result->utime_ticks, &result->stime_ticks,
                &result->cutime_ticks, &result->cstime_ticks, &result->vsize,
                &rss) == EOF) {
        fclose(fpstat);
        return -1;
    }
    fclose(fpstat);
    result->rss = rss * getpagesize();

    //read+calc cpu total time from /proc/stat
    long unsigned int cpu_time[10];
    bzero(cpu_time, sizeof(cpu_time));
    if (fscanf(fstat, "%*s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                &cpu_time[0], &cpu_time[1], &cpu_time[2], &cpu_time[3],
                &cpu_time[4], &cpu_time[5], &cpu_time[6], &cpu_time[7],
                &cpu_time[8], &cpu_time[9]) == EOF) {
        fclose(fstat);
        return -1;
    }

    fclose(fstat);

    for(i=0; i < 4;i++)
        result->cpu_total_time += cpu_time[i];

#ifdef DEBUG
    printf( "usage: cpu %lu, utime %lu, stime %lu\n", result->cpu_total_time, result->utime_ticks, result->stime_ticks );
#endif

    return 0;
}

void calc_cpu_usage_pct(const struct pstat* cur_usage,
                        const struct pstat* last_usage,
                        double* usage)
{
#ifdef DEBUG
    printf( "delta: cpu %lu, utime %lu, stime %lu\n",
#endif
        cur_usage->cpu_total_time - last_usage->cpu_total_time,
        cur_usage->utime_ticks - last_usage->utime_ticks,
        cur_usage->stime_ticks - last_usage->stime_ticks );

    const long unsigned int cpu_diff = cur_usage->cpu_total_time - last_usage->cpu_total_time;
    const long unsigned int pid_diff =
        ( cur_usage->utime_ticks + cur_usage->utime_ticks + cur_usage->stime_ticks - cur_usage->stime_ticks ) -
        ( last_usage->utime_ticks + last_usage->utime_ticks + last_usage->stime_ticks - last_usage->stime_ticks );

    *usage = 100.0 * ( (double)pid_diff / (double)cpu_diff );
}

void test_calc(pid_t pid)
{
    struct pstat prev, curr;
    double pct;

    struct tms t;
    times( &t );

    while( 1 )
    {
        if( get_usage(pid, &prev) == -1 ) {
            printf( "error\n" );
        }

        sleep( 5 );

        if( get_usage(pid, &curr) == -1 ) {
            printf( "error\n" );
        }

        calc_cpu_usage_pct(&curr, &prev, &pct);

        printf("%%cpu: %.02f\n", pct);
    }
}



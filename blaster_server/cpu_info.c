/*                                                                   
Copyright © 2015 NoTalentHack, LLC                            e      
                                                                   
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

// Get CPU Info
void getCpuInfo(struct cpuInfo *cpuinfo)
{
    char buf[BUFSIZE], *ptr;
    int len, found = 0;

    memset(buf, 0, BUFSIZE);
    /* try to open the cpuinfo file */
    FILE* fp = fopen("/proc/cpuinfo", "r");

    if (fp) {
        while ((fgets(buf, sizeof(buf), fp) != NULL) && found < 8) {
            if (strncmp(buf, "vendor_id", 9) == 0) {
               ptr = strstr(buf, ": ");
               if (ptr != '\0') {
                  strcpy(cpuinfo->vendor_id, ptr+2);
                  len = strlen(cpuinfo->vendor_id);
                  if (cpuinfo->vendor_id[len-1] == '\n') {
                     cpuinfo->vendor_id[len-1] = '\0';
                  }
               }
               printf("%s\n", cpuinfo->vendor_id);
               found++;
               memset(buf, 0, BUFSIZE);
               continue;
            }
            if (strncmp(buf, "cpu family", 10) == 0) {
               ptr = strstr(buf, ": ");
               if (ptr != '\0') {
                  cpuinfo->cpu_family = atoi(ptr+2);
               }
               printf("%d\n", cpuinfo->cpu_family);
               found++;
               memset(buf, 0, BUFSIZE);
               continue;
            }
            if (strncmp(buf, "model name", 10) == 0) {
               ptr = strstr(buf, ": ");
               if (ptr != '\0') {
                  strcpy(cpuinfo->model_name, ptr+2);
                  len = strlen(cpuinfo->model_name);
                  if (cpuinfo->model_name[len-1] == '\n') {
                     cpuinfo->model_name[len-1] = '\0';
                  }
               }
               printf("%s\n", cpuinfo->model_name);
               found++;
               memset(buf, 0, BUFSIZE);
               continue;
            }
            if (strncmp(buf, "model", 5) == 0) {
               ptr = strstr(buf, ": ");
               if (ptr != '\0') {
                  cpuinfo->model = atoi(ptr+2);
               }
               printf("%d\n", cpuinfo->model);
               found++;
               memset(buf, 0, BUFSIZE);
               continue;
            }
            if (strncmp(buf, "cpu cores", 9) == 0) {
               ptr = strstr(buf, ": ");
               if (ptr != '\0') {
                  cpuinfo->cpu_cores = atoi(ptr+2);
               }
               printf("%d\n", cpuinfo->cpu_cores);
               found++;
               memset(buf, 0, BUFSIZE);
               continue;
            }
            if (strncmp(buf, "stepping", 8) == 0) {
               ptr = strstr(buf, ": ");
               if (ptr != '\0') {
                  cpuinfo->stepping = atoi(ptr+2);
               }
               printf("%d\n", cpuinfo->stepping);
               found++;
               memset(buf, 0, BUFSIZE);
               continue;
            }
            if (strncmp(buf, "cpu MHz", 7) == 0) {
               ptr = strstr(buf, ": ");
               if (ptr != '\0') {
                  cpuinfo->frequency = atof(ptr+2);
               }
               printf("%f\n", cpuinfo->frequency);
               found++;
               memset(buf, 0, BUFSIZE);
               continue;
            }
            if (strncmp(buf, "cache size", 10) == 0) {
               ptr = strstr(buf, ": ");
               if (ptr != '\0') {
                  strcpy(cpuinfo->cache_size, ptr+2);
                  len = strlen(cpuinfo->cache_size);
                  if (cpuinfo->cache_size[len-1] == '\n') {
                     cpuinfo->cache_size[len-1] = '\0';
                  }
               }
               printf("%s\n", cpuinfo->cache_size);
               found++;
               memset(buf, 0, BUFSIZE);
               continue;
            }
        }
        fclose(fp);
    }
}


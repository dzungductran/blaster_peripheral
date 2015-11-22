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
#include "cJSON/cJSON.h"

uint8_t channel = 22;

#ifdef _DEBUG
#define DEBUG 1
#else
#define DEBUG 0
#endif

#define DEBUG 1

// commands between Client and Server. Server is on Edison side and
// client is on the phone side
#define SERIAL_CMD_START       1
#define SERIAL_CMD_STATUS      2
#define SERIAL_CMD_ERROR       3
#define SERIAL_CMD_KILL        9       // equals to SIGKILL
#define SERIAL_CMD_STOP        17      // equals to SIGSTOP
#define SERIAL_CMD_CPU_INFO    6
#define SERIAL_CMD_TERM        15      // equals to SIGTERM
#define SERIAL_CMD_CLOSE       0xFF 

const char *KEY_COMMAND_TYPE   = "command_type";
const char *KEY_COMMAND        = "command";
const char *KEY_CAPTURE_OUTPUT = "capture_output";
const char *KEY_IDENTIFIER     = "identifier";
const char *KEY_PERCENT        = "percent";
const char *KEY_TOAST          = "toast";
const char *KEY_FREQUENCY      = "frequency";
const char *KEY_CPU_CORES      = "cpu_cores";
const char *KEY_CPU_FAMILY     = "cpu_family";
const char *KEY_STEPPING       = "stepping";
const char *KEY_MODEL          = "model";
const char *KEY_CACHE_SIZE     = "cache_size";
const char *KEY_MODEL_NAME     = "model_name";
const char *KEY_VENDOR_ID      = "vendor_id";
const char *KEY_PROCESS_STATE  = "process_state";
const char *KEY_QUICK_STATUS   = "quick_status";


/* Protocol between client and server
 *     byte 1 = command 
 *     byte 2-n = command line except for close which is empy
 */

// strip out all arguments                                        
void strip_argv(char *buffer) {                                   
    char *ptr;                                                    
    int slen;                                                     
                                                                  
    ptr = strchr(buffer, ' ');  // space is where arguments starts
    if (ptr != '\0') {                                            
        slen = (int)(ptr - buffer);                                   
        if (slen > 0) {                                               
            buffer[slen] = '\0';                                      
        }                                                             
        printf("buf = %s\n", buffer);                                 
    }                                                             
} 

void returnCpuInfo(int client) {
    struct cpuInfo cpuinfo;
    getCpuInfo(&cpuinfo); 

    // build cpu info data
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, KEY_COMMAND_TYPE, SERIAL_CMD_CPU_INFO);
    cJSON_AddNumberToObject(json, KEY_FREQUENCY, cpuinfo.frequency);
    cJSON_AddNumberToObject(json, KEY_CPU_CORES, cpuinfo.cpu_cores);
    cJSON_AddNumberToObject(json, KEY_CPU_FAMILY, cpuinfo.cpu_family);
    cJSON_AddNumberToObject(json, KEY_STEPPING, cpuinfo.stepping);
    cJSON_AddNumberToObject(json, KEY_MODEL, cpuinfo.model);
    cJSON_AddStringToObject(json, KEY_CACHE_SIZE, cpuinfo.cache_size);
    cJSON_AddStringToObject(json, KEY_MODEL_NAME, cpuinfo.model_name);
    cJSON_AddStringToObject(json, KEY_VENDOR_ID, cpuinfo.vendor_id);
        
    char *out = cJSON_Print(json, 0);
    int slen = strlen(out);
    int bytes_wrote = write(client, out, slen);              
    if (bytes_wrote <= 0 || bytes_wrote != slen) {       
        // Use system log?                                 
        fprintf(stderr, "Can't write to client\n");        
    }                                                      
    free(out);
    cJSON_Delete(json);
}

// return unknown command error message
void returnMessage(int client, int msgType,  char *format, char *str) {
    char error[256] = { 0 };
    sprintf(error, format, str);

    // build status data
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, KEY_COMMAND_TYPE, msgType);
    cJSON_AddStringToObject(json, KEY_TOAST, error);
        
    char *out = cJSON_Print(json, 0);
    int slen = strlen(out);
    int bytes_wrote = write(client, out, slen);              
    if (bytes_wrote <= 0 || bytes_wrote != slen) {       
        // Use system log?                                 
        fprintf(stderr, "Can't write to client\n");        
    }                                                      
    free(out);
    cJSON_Delete(json);
} 

// return last known error message
void returnOutput(int client) {
    char error[BUFSIZE] = { 0 };
    get_lasterror(error);                         
    returnMessage(client, SERIAL_CMD_ERROR, "%s", error);
}

void returnUnknown(int client, char *format, char *str) {
    returnMessage(client, SERIAL_CMD_ERROR, format, str);
}

// argv that is needed to pass into returnCpuUsage function as part of
// pthread_create function
struct argvCpuInfo {
    int client;
    int identifier;
    int quick;
    pid_t pid;
}; 

void returnCpuUsage(int client, int identifier, int quick, double pct, char *state) 
{
    // build status data
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, KEY_COMMAND_TYPE, SERIAL_CMD_STATUS);
    cJSON_AddNumberToObject(json, KEY_IDENTIFIER, identifier);
    cJSON_AddStringToObject(json, KEY_PROCESS_STATE, state);
    cJSON_AddNumberToObject(json, KEY_PERCENT, pct);
    cJSON_AddNumberToObject(json, KEY_QUICK_STATUS, quick);

    char *out = cJSON_Print(json, 0);
    int slen = strlen(out);
    int bytes_wrote = write(client, out, slen);
    if (bytes_wrote <= 0 || bytes_wrote != slen) {
        // Use system log?
        fprintf(stderr, "Can't write to client\n");
    }
#ifdef DEBUG
    printf("%s\n", out);
#endif
    free(out);
    cJSON_Delete(json);
    printf("%%cpu: %.02f\n", pct);
}

// Function to return cpu usage and other info about process
void* getCpuUsage(void *arg)
{
    struct pstat prev, curr;
    double pct = 0;
    char state[2] = {'X', '\0'};
    struct argvCpuInfo *argv = (struct argvCpuInfo *)arg;
    int cnt = 0;

    // could loop if needed by change cnt < n
    while( cnt < 1 )
    {
        if( get_usage(argv->pid, &prev) != -1 ) {
            if (argv->quick == 0) { // if quick status then don't calculate cpu usage
                sleep( 2 );

                if ( get_usage(argv->pid, &curr) != -1 ) {
                   calc_cpu_usage_pct2(&curr, &prev, &pct);
                   state[0] = curr.state;
                }
            } else {
                state[0] = prev.state;
            }
        }

        returnCpuUsage(argv->client, argv->identifier, argv->quick, pct, state);

        cnt++;
        printf("%%cpu: %.02f\n", pct);
    }

    free(argv);
    return 0;
}

int main(int argc, char **argv)
{
    struct sockaddr_rc loc_addr = { 0 }, rem_addr = { 0 };
    char buf[1024] = { 0 };
    char client_addr[64];
    int sock, client, bytes_read, status;
    socklen_t opt = sizeof(rem_addr);
    pid_t pid;
    cJSON *json;

    // allocate socket
    sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (sock < 0) {
       fprintf(stderr, "Can't open bluetooth connection\n");
       exit(-1);
    }

    // bind socket to port 1 of the first available 
    // local bluetooth adapter
    loc_addr.rc_family = AF_BLUETOOTH;
    loc_addr.rc_bdaddr = *BDADDR_ANY;
    loc_addr.rc_channel = channel;
    if (bind(sock, (struct sockaddr *)&loc_addr, sizeof(loc_addr)) < 0) {
       fprintf(stderr, "Can't bind to bluetooth connection\n");
       exit(-1);
    }

    // set the listening queue length
    // put socket into listening mode
    if (listen(sock, 1) < 0) {
       fprintf(stderr, "Can't listen to bluetooth connection\n");
       exit(-1);
    }

    while(1) {
    	printf("Accepting connections on channel: %d\n", channel);

    	// accept one connection
    	client = accept(sock, (struct sockaddr *)&rem_addr, &opt);

    	ba2str( &rem_addr.rc_bdaddr, client_addr );
    	printf("Accepted connection from %s\n", client_addr);

        int done = 0;
    	while (!done) {
    	    // read data from the client
            printf("Waiting for data from %s\n", client_addr);
    	    memset(buf, 0, sizeof(buf));
    	    bytes_read = read(client, buf, sizeof(buf));

    	    if( bytes_read > 0 ) {
#ifdef DEBUG
                printf("len=%d buf=%s\n", bytes_read, buf);
#endif
                json = cJSON_Parse(buf);
                int cmd = cJSON_GetObjectItem(json, KEY_COMMAND_TYPE)->valueint;
                switch (cmd) 
                {
                    case SERIAL_CMD_START:
                    { 
                        char *oType = cJSON_GetObjectItem(json, KEY_CAPTURE_OUTPUT)->valuestring;
                        char *cmdStr = cJSON_GetObjectItem(json, KEY_COMMAND)->valuestring;
    		        status = shellcmd(cmdStr, oType);  // read error from stderr
#ifdef DEBUG
                        printf("status = %d for command %s type %s\n", status, cmdStr, oType);
#endif
                        if (status != 0 || *oType == 'r' || *oType == 'o') {
                            returnOutput(client);
                        }

                        break;
                    } 
                    case SERIAL_CMD_KILL:
                    {
                        char *cmdStr = cJSON_GetObjectItem(json, KEY_COMMAND)->valuestring;
			strcpy(buf, cmdStr);
                    	strip_argv(buf); 
                    	pid = findCommand(buf);
                    	if (pid != -1) {
                            status = kill(pid, SIGKILL);
                            if (status == -1) {
                                returnOutput(client);
                            }
                        } else {
                            returnUnknown(client, "Can't find %s", cmdStr);
             		}
                        break;
                    }
                    case SERIAL_CMD_TERM:
                    {
                        char *cmdStr = cJSON_GetObjectItem(json, KEY_COMMAND)->valuestring;
			strcpy(buf, cmdStr);
                        strip_argv(buf); 
                        pid = findCommand(buf);
                        if (pid != -1) {
                            status = kill(pid, SIGTERM);
                            if (status == -1) {
                                returnOutput(client);
                            }
                        } else {
                            returnUnknown(client, "Can't find %s", cmdStr);
                        }
                        break;
                    }
                    case SERIAL_CMD_STOP:
                    {
                        char *cmdStr = cJSON_GetObjectItem(json, KEY_COMMAND)->valuestring;
			strcpy(buf, cmdStr);
                        strip_argv(buf); 
                        pid = findCommand(buf);
                        if (pid != -1) {
                            status = kill(pid, SIGSTOP);
                            if (status == -1) {
                                returnOutput(client);
                            }
                        } else {
                            returnUnknown(client, "Can't find %s", cmdStr);
                        }
                        break;
                    }
                    case SERIAL_CMD_STATUS: // get status for a command
                    {
                        // get status from /proc/stat
                        char *cmdStr = cJSON_GetObjectItem(json, KEY_COMMAND)->valuestring;
                        int id = cJSON_GetObjectItem(json, KEY_IDENTIFIER)->valueint;
                        int quick = cJSON_GetObjectItem(json, KEY_QUICK_STATUS)->valueint;
#ifdef DEBUG
                        printf("command %s id %d\n", cmdStr, id);
#endif
			strcpy(buf, cmdStr);
                        strip_argv(buf); 
                        pid = findCommand(buf);
                        printf("pid %d for command %s\n", pid, buf);
                        if (pid != -1) {
                            pthread_t tid;
                            struct argvCpuInfo *arg = malloc(sizeof(struct argvCpuInfo));
                            arg->client = client;
                            arg->pid = pid;
                            arg->identifier = id;
                            arg->quick = quick;
                            if (pthread_create(&tid, NULL, getCpuUsage, arg) != 0) {
                                returnUnknown(client, "Can't stat %", cmdStr);
                            }
                        } else {
                            // can't find pid and we looking for quick status so return not_running
                            if (quick) {
                                 returnCpuUsage(client, id, quick, 0, "X");
                            } else {
                                 returnUnknown(client, "Can't stat %s", cmdStr);
                            }
                        }
                        break;
                    }
                    case SERIAL_CMD_CPU_INFO:
                    {
                        returnCpuInfo(client);
                        break;

                    }
                    case SERIAL_CMD_CLOSE:
                    {
                        done = 1;
                        break;
                    }
                    default:
                        returnUnknown(client, "Don't understand command %s", cJSON_Print(json,0));
                        
                }
            } // if bytes_read > 0
            else 
            {
                done = 1;
            }
               
        } // while (!done)

        printf("Close connection to %s\n", client_addr);
    	// close connection
    	close(client);
    } // while(1)

    close(sock);
    exit(0);
}

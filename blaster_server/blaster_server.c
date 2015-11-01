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
#define SERIAL_CMD_KILL        4 
#define SERIAL_CMD_STOP        5
#define SERIAL_CMD_CPU_INFO    6
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
void returnError(int client) {
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
    pid_t pid;
}; 

// Function to return cpu usage and other info about process
void* returnCpuUsage(void *arg)                                                                                                
{                                                                                                                        
    struct pstat prev, curr;                                                                                             
    double pct;                                                                                                          
    struct argvCpuInfo *argv = (struct argvCpuInfo *)arg; 
    int cnt = 0;
    
    // could loop if needed by change cnt < n
    while( cnt < 1 )
    {                                                                                                                    
        if( get_usage(argv->pid, &prev) == -1 ) {                                                                              
            printf( "error\n" );                                                                                         
        }                                                                                                                
                                                                                                                         
        sleep( 2 );                                                                                                      
                                                                                                                         
        if( get_usage(argv->pid, &curr) == -1 ) {                                                                              
            printf( "error\n" );                                                                                         
        }                                                                                                                
                                                                                                                         
        calc_cpu_usage_pct(&curr, &prev, &pct);                                                                          
       
        // build status data
        cJSON *json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, KEY_COMMAND_TYPE, SERIAL_CMD_STATUS);
        cJSON_AddNumberToObject(json, KEY_IDENTIFIER, argv->identifier);
        cJSON_AddNumberToObject(json, KEY_PERCENT, pct);
        
        int slen = strlen(json->string);
        int bytes_wrote = write(argv->client, json->string, slen);              
        if (bytes_wrote <= 0 || bytes_wrote != slen) {       
           // Use system log?                                 
           fprintf(stderr, "Can't write to client\n");        
        }                                                      
        cJSON_Delete(json);

        cnt++;
        printf("%%cpu: %.02f\n", pct);                                                                                   
    }                                                                                                                    

    free(argv);
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
                        if (status !=0) {
                            returnError(client);
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
                                returnError(client);
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
                                returnError(client);
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
			strcpy(buf, cmdStr);
                        strip_argv(buf); 
                        pid = findCommand(buf);
                        if (pid != -1) {
                            pthread_t tid;
                            struct argvCpuInfo *arg = malloc(sizeof(struct argvCpuInfo));
                            arg->client = client;
                            arg->pid = pid;
                            arg->identifier = id;
                            if (pthread_create(&tid, NULL, returnCpuUsage, arg) != 0) {
                                returnUnknown(client, "Can't stat %", cmdStr);
                            }
                        } else {
                            returnUnknown(client, "Can't stat %s", cmdStr);
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

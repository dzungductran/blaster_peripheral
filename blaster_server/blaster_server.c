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

uint8_t channel = 22;

#ifdef _DEBUG
#define DEBUG 1
#else
#define DEBUG 0
#endif

// commands between Client and Server. Server is on Edison side and
// client is on the phone side
const unsigned char SERIAL_CMD_START   = 0x1;
const unsigned char SERIAL_CMD_STATUS  = 0x2;
const unsigned char SERIAL_CMD_ERROR   = 0x3;
const unsigned char SERIAL_CMD_KILL    = 0x4;
const unsigned char SERIAL_CMD_STOP    = 0x5;
const unsigned char SERIAL_CMD_CLOSE   = 0xF;

/* Protocol between client and server
 *     byte 1 = command 
 *     byte 2-n = command line except for close which is empy
 */

// convert int to 4 bytes
void convert_int4bytes(int n, char *bytes) {
    bytes[0] = (n >> 24) & 0xFF;
    bytes[1] = (n >> 16) & 0xFF;
    bytes[2] = (n >> 8) & 0xFF;
    bytes[3] = n & 0xFF;
}

int convert_4bytesint(char *bytes) {
    int x = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    return x;
}

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

// return last known error message
void returnError(int client) {
    char error[256] = { 0 };

    error[0] = SERIAL_CMD_ERROR;                             
    int slen = get_lasterror(&error[1]);                         
    int bytes_wrote = write(client, error, slen+1);              
    if (bytes_wrote <= 0 || bytes_wrote != slen+1) {       
       // Use system log?                                 
       fprintf(stderr, "Can't write to client\n");        
    }                                                      
}

// return unknown command error message
void returnUnknown(int client, char *format, char *str) {
    char error[256] = { 0 };

    error[0] = SERIAL_CMD_ERROR;                             
    sprintf(&error[1], format, str);
    int slen = strlen(&error[1]);
    int bytes_wrote = write(client, error, slen+1);              
    if (bytes_wrote <= 0 || bytes_wrote != slen+1) {       
       // Use system log?                                 
       fprintf(stderr, "Can't write to client\n");        
    }                                                      
} 

// argv that is needed to pass into returnCpuUsage function as part of
// pthread_create function
struct argvCpuInfo {
    int client;
    pid_t pid;
}; 

// Function to return cpu usage and other info about process
void* returnCpuUsage(void *arg)                                                                                                
{                                                                                                                        
    struct pstat prev, curr;                                                                                             
    double pct;                                                                                                          
    struct argvCpuInfo *argv = (struct argvCpuInfo *)arg; 
    
    while( 1 )                                                                                                           
    {                                                                                                                    
        if( get_usage(argv->pid, &prev) == -1 ) {                                                                              
            printf( "error\n" );                                                                                         
        }                                                                                                                
                                                                                                                         
        sleep( 2 );                                                                                                      
                                                                                                                         
        if( get_usage(argv->pid, &curr) == -1 ) {                                                                              
            printf( "error\n" );                                                                                         
        }                                                                                                                
                                                                                                                         
        calc_cpu_usage_pct(&curr, &prev, &pct);                                                                          
                                                                                                                         
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
                int i;
                for (i=0; i<bytes_read; i++)
                    printf("received [%x]", buf[i]);
                printf("\n");
#endif
                if ( buf[0] == SERIAL_CMD_START )
                {
    		    status = shellcmd(&buf[1], "e");  // read error from stderr
                    if (status !=0) {
                        returnError(client);
                    } 
                } 
                else if ( buf[0] == SERIAL_CMD_KILL ) {
                    strip_argv(buf); 
                    pid_t pid = findCommand(buf);
                    if (pid != -1) {
                        status = kill(pid, SIGKILL);
                        if (status == -1) {
                            returnError(client);
                        }
                    } else {
                       returnUnknown(client, "Can't find %s", &buf[1]);
		    }
                }
                else if ( buf[0] == SERIAL_CMD_STOP ) {
                    strip_argv(buf); 
                    pid_t pid = findCommand(buf);
                    if (pid != -1) {
                        status = kill(pid, SIGSTOP);
                        if (status == -1) {
                            returnError(client);
                        }
                    } else {
                       returnUnknown(client, "Can't find %s", &buf[1]);
                    }
                }
                else if ( buf[0] == SERIAL_CMD_STATUS )
                {
                    // get status from /proc/stat
                    strip_argv(buf); 
                    pid_t pid = findCommand(buf);
                    if (pid != -1) {
                        pthread_t tid;
                        struct argvCpuInfo *arg = malloc(sizeof(struct argvCpuInfo));
                        arg->client = client;
                        arg->pid = pid;
                        if (pthread_create(&tid, NULL, returnCpuUsage, arg) != 0) {
                            returnUnknown(client, "Can't stat %", buf);
                        }
                    } else {
                        returnUnknown(client, "Can't stat %s", &buf[1]);
                    }
                }
                else if ( buf[0] == SERIAL_CMD_CLOSE )
                {
                    done = 1;
                }
                else {
		    // system log?
                    fprintf(stderr, "Unknown message %s\n", buf); 
                }
            }
               
        }

        printf("Close connection to %s\n", client_addr);
    	// close connection
    	close(client);
    }

    close(sock);
    exit(0);
}

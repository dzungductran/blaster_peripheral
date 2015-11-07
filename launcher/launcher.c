/******************************************************************************
** Supports GPIO to turn on/off relay

Development environment specifics:
  This code requires the Intel mraa library to function; for more
  information see https://github.com/intel-iot-devkit/mraa

Distributed as-is; no warranty is given.
******************************************************************************/

#include <mraa.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

/* Global iopin to write to */
int iopin = MRAA_INTEL_EDISON_GP46;                                          

void write_pin(int data) 
{
    mraa_gpio_context gpio;
    mraa_result_t result = MRAA_SUCCESS;

    gpio = mraa_gpio_init(iopin);                                                
    mraa_gpio_dir(gpio, MRAA_GPIO_OUT);                                              
                                                                                 
    result = mraa_gpio_write(gpio, data);                                           
    if (result != MRAA_SUCCESS) {                                                
       syslog (LOG_ERR, "Error writing to pin %d\n", iopin);                     
       exit(1);                                                                  
    }                                                                            
    syslog(LOG_INFO, "Gpio is %d", mraa_gpio_read(gpio));
    mraa_gpio_close(gpio);                                                       
}

/* sign handler where we will stop writing to pin */               
void sig_handler(int signo)                                        
{                                                                  
    if (signo == SIGINT || signo == SIGTERM || signo == SIGHUP) {  
        printf("Closing and cleaning up\n");                       
        write_pin(0);                                              
    }                                                                            
}                                                                                
                                                                                 
int main( int argc, char* argv[] )
{
    /* Our process ID and Session ID */
    pid_t pid, sid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Can't fork child\n");
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
              we can exit the parent process. */
    if (pid > 0) {
       exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);

    setlogmask (LOG_UPTO (LOG_NOTICE));
    openlog (argv[0], LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    syslog (LOG_NOTICE, "Program started by User %d", getuid ());

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        /* Log the failure */
        syslog (LOG_ERR, "Can't create new SID for child process");
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory */
    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGHUP, sig_handler);
    
    int iopin = MRAA_INTEL_EDISON_GP46;
    if (argc > 1) {
        iopin = atoi(argv[1]);
    }

    write_pin(1);
    pause();  // wait for signal

    exit(0);
}

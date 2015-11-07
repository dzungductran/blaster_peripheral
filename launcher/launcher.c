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

int main( int argc, char* argv[] )
{
    mraa_gpio_context gpio;
    mraa_result_t result = MRAA_SUCCESS;
    gpio = mraa_gpio_init(MRAA_INTEL_EDISON_GP46);
    mraa_gpio_dir(gpio, MRAA_GPIO_OUT);
    for (;;) {
/*        result = mraa_gpio_write(gpio, 1);
        if (result != MRAA_SUCCESS) {
            fprintf(stderr, "Error writing to pin %d\n", MRAA_INTEL_EDISON_GP46);
            break;
        } */
        fprintf(stdout, "Gpio is %d\n", mraa_gpio_read(gpio));
        sleep(1);
        result = mraa_gpio_write(gpio, 0);
        if (result != MRAA_SUCCESS) {
            fprintf(stderr, "Error writing to pin %d\n", MRAA_INTEL_EDISON_GP46);
            break;
        }
        break;
    }
    mraa_gpio_close(gpio);
    return result;
}

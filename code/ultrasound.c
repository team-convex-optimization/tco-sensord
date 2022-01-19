#include "ultrasound.h"

sensor_ultrasound *us_init(int gpio_trig, int gpio_echo)
{
    sensor_ultrasound *us = (sensor_ultrasound *)malloc(sizeof(sensor_ultrasound));
    us->echo = (gpio_handle_t *)malloc(sizeof(gpio_handle_t));
    us->trig = (gpio_handle_t *)malloc(sizeof(gpio_handle_t));

    error_t err1 = gpio_handle_get(us->trig, 0, GPIO_DIR_OUT, gpio_trig);
    error_t err2 = gpio_handle_get(us->echo, 0, GPIO_DIR_IN, gpio_echo);

    if ((err1 | err2) != ERR_OK)
    {
        log_error("Could not initialize ultrasonic sensor");
        return NULL;
    }
    log_info("Ultrasound sensor has been correctly initialized");
    return us;
}

double us_get_distance(sensor_ultrasound *us) /* TODO : Check err codes */
{
    struct timespec start_spec, end_spec;
    enum gpio_val val;

    /* Send pulse */
    gpio_line_write(us->trig, GPIO_VAL_HIGH);
    usleep(10000);
    gpio_line_write(us->trig, GPIO_VAL_LOW);

    /* Receive pulse */
    do {
        gpio_line_read(us->echo, &val);
    } while (val == GPIO_VAL_LOW);
    clock_gettime(_POSIX_MONOTONIC_CLOCK, &start_spec);
    do {
        gpio_line_read(us->echo, &val);
    } while (val == GPIO_VAL_LOW);
    clock_gettime(_POSIX_MONOTONIC_CLOCK, &end_spec);

    /* Calculate the length of the pulse in seconds */
    long double rtt = ((end_spec.tv_nsec - start_spec.tv_nsec)/NANO_SEC_TO_SEC);
    double distance = rtt * SPEED_OF_SOUND_CM_HALF;
    return distance;
}

void us_cleanup(sensor_ultrasound *us)
{
    gpio_line_close(us->echo);
    gpio_line_close(us->trig);
    free(us);
    log_info("Ultrasound sensor has been cleaned up");
}

void us_test(int gpio_trig, int gpio_echo, int num_pings)
{
    sensor_ultrasound *us = (sensor_ultrasound *)us_init(gpio_trig, gpio_echo);
    if (us == NULL)
    {
	log_error("us_test failed as ultrasound sensor initialization failed");
	return;
    }
    for (int i = 0; i < num_pings; i++)
    {
        double dist = us_get_distance(us);
        printf("Ping %d has distance of %f cm(s)\n", i, dist);
	    usleep(500000);
    }
    printf("\nTest complete. cleaning...\n");
    us_cleanup(us);
}

void us_usage() {
    printf("Ultrasound sensor test mode:\n"
           "Run the program with './<program> -u <trigger_pin> <echo_pin> <num_pings>' where:\n"
           "trigger_pin: The GPIO pin where trig is connected to.\n"
           "echo_pin: The GPIO pin where echo is connected to.\n"
           "num_pings: The number of pings you want to test.\n"
           "EXAMPLE: './program 16 18 1000'\n");
}
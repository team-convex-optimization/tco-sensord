#include "ultrasound.h"
#include <gpiod.h>

void *us_init(const void **args)
{
    int gpio_trig = ((int *) *args)[0];
    int gpio_echo = ((int *) *args)[1];
	log_info("US being insantiated with echo : %d and trig : %d", gpio_trig, gpio_echo);
	sensor_ultrasound *us = (sensor_ultrasound *)malloc(sizeof(sensor_ultrasound));
    us->echo = (gpio_handle_t *)malloc(sizeof(gpio_handle_t));
    us->trig = (gpio_handle_t *)malloc(sizeof(gpio_handle_t));
    
    error_t err1 = gpio_handle_get(us->trig, gpio_trig/32, GPIO_DIR_OUT, gpio_trig%32);
    error_t err2 = gpio_handle_get(us->echo, gpio_echo/32, GPIO_DIR_NONE, gpio_echo%32);

    if ((err1 | err2) != ERR_OK)
    {
        log_error("Could not initialize ultrasonic sensor");
        return NULL;
    }

    const char *consumer = gpiod_line_consumer(us->echo->line);
    if (gpiod_line_request_both_edges_events(us->echo->line, consumer) != 0) {
        log_error("Failed to request events on echo pin %d", gpio_echo);
    }

    log_info("Ultrasound sensor has been correctly initialized");
    return (void *)us;
}

double get_and_add_average(sensor_ultrasound *us, double value) {
    double avg = value;
    for (int i = 1; i < AVG_FILTER_SIZE; i++) {
        us->prev_measures[i-1] = us->prev_measures[i]; 
        avg += us->prev_measures[i];
    }
    us->prev_measures[AVG_FILTER_SIZE-1] = value;
    return avg/AVG_FILTER_SIZE;
}

double us_get_distance(sensor_ultrasound *us) 
{
    struct timespec start_spec, end_spec, timeout = {0, 100000000};
    struct gpiod_line_event event = {};

    /* Send pulse */
    if (gpio_line_write(us->trig, GPIO_VAL_LOW) != ERR_OK) goto fail;
    usleep(2); /* Clear the state of the pin */
    if (gpio_line_write(us->trig, GPIO_VAL_HIGH) != ERR_OK) goto fail;
    usleep(10);
    gpio_line_write(us->trig, GPIO_VAL_LOW);
    
    /* Receive pulse */
    int ret = gpiod_line_event_wait(us->echo->line, &timeout);
    switch (ret)
    {
        case -1:
            log_error("failed to get event on us_echo line");
            goto fail;
            break;
        case 1:
            gpiod_line_event_read(us->echo->line, &event);
            clock_gettime(_POSIX_MONOTONIC_CLOCK, &start_spec);
            break;
        default: /* timeout */
            goto fail;
    }
    	
    ret = gpiod_line_event_wait(us->echo->line, &timeout);
    switch (ret)
    {
        case -1:
            log_error("failed to get event on us_echo line");
            goto fail;
            break;
        case 1:
            gpiod_line_event_read(us->echo->line, &event);
            clock_gettime(_POSIX_MONOTONIC_CLOCK, &end_spec);
            break;
        default: /* timeout */
            goto fail;
    }

    /* Calculate the length of the pulse in seconds */
    long double rtt = ((end_spec.tv_nsec - start_spec.tv_nsec)/NANO_SEC_TO_SEC);
    double distance = rtt * SPEED_OF_SOUND_CM_HALF;
    if (distance <= 4.0) goto fail;
    // printf("distance is %f\n", (float)distance);
    return get_and_add_average(us, distance);

fail:
    log_error("Failed to read/write to GPIO pin");
    return 0.0;
}

void us_cleanup(sensor_ultrasound *us)
{
    gpio_line_close(us->echo);
    gpio_line_close(us->trig);
    free(us);
    log_info("Ultrasound sensor has been cleaned up");
}
/*
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
*/
void us_usage() {
    printf("Ultrasound sensor test mode:\n"
           "Run the program with './<program> -u <trigger_pin> <echo_pin> <num_pings>' where:\n"
           "trigger_pin: The GPIO pin where trig is connected to.\n"
           "echo_pin: The GPIO pin where echo is connected to.\n"
           "num_pings: The number of pings you want to test.\n"
           "EXAMPLE: './program 16 18 1000'\n");
}

#ifndef SENSOR_H_
#define SENSOR_H_

#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "tco_libd.h"

/**
 * @brief This stucture contains a sensor module that will be pulled by the main loop. Every parameter 
 * requires a specific format. Each of these sensors is run in a seperate thread.
 * @param reference is a pointer to the object reference. Leave this unchanged. 
 * @param clean_up is a reference to the function to cleanup any resources a sensor's object may have
 * taken
 * @param init is a function reference to the init method for the sensor. It must have form init(void **args) and the args are provided by init_args. 
 * @param init_args is a malloc´d reference to the function paramaters expected by the sensor.
 * @param interval is the number of useconds to wait before reading a new value
 */
typedef struct {
	void *reference;
	void (*cleanup)(void); /* A function to cleanup the object */
	void *(*init)(void **); /* A function to initialize the object */
	double *(*read)(void *); /* A function to read the value. parameter must be reference and return a double */
	void **init_args;
	unsigned int interval;
} sensor_t;

/**
 * @brief a parent structure that encapsulates all sensors. This is used to initialize, clean and create threads
 * for each sensor.
 * @param count is the number of sensors in the object
 * @param sensors *is a malloc´d array for every sensor from 0 to count-1
 */
typedef struct {
	uint8_t count;
	sensor_t *sensors;
} sensors_t;


/*
 * @brief add a sensor to the monolothic sensor structure
 * @param init a ptr to the init function
 * @param init_args a ptr to a malloc´d array needed for the init of the sensor function
 * @param cleanup a ptr to the cleanup function
 * @param interval the number of useconds to wait between polls on the sensor
 */
int add_sensor(void *init, void **init_args, void *cleanup, unsigned int interval);

/* 
 * @brief initialize all sensors in the sensor struct. This includes giving them all to a seperate thread.
 */
int initialize_sensors();

/*
 * @brief cleanup all sensors in the sensor struct
 */
int cleanup_sensors();

#endif /* SENSOR_H_ */

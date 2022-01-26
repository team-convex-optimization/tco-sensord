#ifndef HALL_EFFECT_H_
#define HALL_EFFECT_H_

#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include "tco_libd.h"

#define NANO_SEC_TO_SEC 1000000000.0f
#define MOTOR_TO_WHEEL_RATIO 3.2f
#define NUMBER_OF_POLES 3.0f
#define WAIT_LIMIT 30000000UL

#define FILTER_SIZE 3

typedef struct {
	gpio_handle_t *pole;
	float prev_results[5];
} sensor_halleffect_t;

/**
 * @brief will initialize an ultrasound sensor with given GPIO port
 * @param args is a malloc´d ptr to an array of parameter 
 * 	@param (@p args[0]) pole which is pin number where to sense for hall_effect_high state
 * @return NULL on failure, void * to initialized object on success
 */
void *hall_effect_init(const void **args);

/**
 * @brief returns the RPM of the motor 
 * @param he is the reference to an initialized hall_effect sensor
 * @return the RPM of the pole
 */
double hall_effect_read(sensor_halleffect_t *he);

/**
 * @brief Clears the resources used by the sensor object
 * @param he is the ptr to the sensor
 */
void hall_effect_cleanup(sensor_halleffect_t *he);

#endif /* HALL_EFFECT_H_ */

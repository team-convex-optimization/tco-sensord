#include "hall_effect.h"
#include <stdio.h>

void *hall_effect_init(const void **args) {
	log_info("Hall Effect sensor is being initialized");
	int he_pin = ((int *) *args)[0];

	sensor_halleffect_t *he = (sensor_halleffect_t *)malloc(sizeof(sensor_halleffect_t));
	he->pole = (gpio_handle_t *)malloc(sizeof(gpio_handle_t));
	error_t err = gpio_handle_get(he->pole, he_pin/32, GPIO_DIR_IN, he_pin%32);

	if (err != ERR_OK) {
		log_error("Failed to initialize the hall_effect sensor on pin %d", he_pin);
		goto fail;
	}

	log_info("Hall Effect sensor has been initialized");
	return (void *)he;

fail:
	free(he->pole);
	free(he);
	return NULL;
}

void add_to_results(sensor_halleffect_t *he, double val) {
	for (int i = 1; i < FILTER_SIZE; i++) {
		he->prev_results[i - 1] = he->prev_results[i];
	}
	he->prev_results[FILTER_SIZE - 1] = val;
}

double get_avg_result(sensor_halleffect_t *he) {
	double res = 0;
	for (int i = 0; i < FILTER_SIZE; i++) {
		res += he->prev_results[i];
	}
	return res/(double)FILTER_SIZE;
}

double hall_effect_read(sensor_halleffect_t *he) {
	struct timespec start_spec, end_spec, timeout_spec, current_spec;
	enum gpio_val val;

	/* Wait until low state */
	clock_gettime(_POSIX_MONOTONIC_CLOCK, &timeout_spec);
	do {
		if(gpio_line_read(he->pole, &val) != ERR_OK)
			goto fail;

		clock_gettime(_POSIX_MONOTONIC_CLOCK, &current_spec);
		if (current_spec.tv_nsec - timeout_spec.tv_nsec > WAIT_LIMIT)
				goto still;
	} while (val == GPIO_VAL_HIGH);
	
	/* Wait for high state to start */
	clock_gettime(_POSIX_MONOTONIC_CLOCK, &timeout_spec);
	do {
		if(gpio_line_read(he->pole, &val) != ERR_OK)
			goto fail;
		clock_gettime(_POSIX_MONOTONIC_CLOCK, &current_spec);
		if (current_spec.tv_nsec - timeout_spec.tv_nsec > WAIT_LIMIT)
				goto still;
	} while (val == GPIO_VAL_LOW);
	clock_gettime(_POSIX_MONOTONIC_CLOCK, &start_spec);

	/* Wait for high state to end */
	clock_gettime(_POSIX_MONOTONIC_CLOCK, &timeout_spec);
	do {
		if(gpio_line_read(he->pole, &val) != ERR_OK)
			goto fail;
		clock_gettime(_POSIX_MONOTONIC_CLOCK, &current_spec);
		if (current_spec.tv_nsec - timeout_spec.tv_nsec > WAIT_LIMIT)
				goto still;
	} while (val == GPIO_VAL_HIGH);
	
	clock_gettime(_POSIX_MONOTONIC_CLOCK, &end_spec);
	
	/* Calculate frequency */
	long double time = ((end_spec.tv_nsec - start_spec.tv_nsec)/NANO_SEC_TO_SEC);
	time *= 2; /* Get period -> get high state length * 2 for full wave_length */
	if (time < 0) time = 1000.0f; 
	double res =  (1.0/time) * MOTOR_TO_WHEEL_RATIO * NUMBER_OF_POLES; /* Freq = 1 / period */
	add_to_results(he, res);
	return get_avg_result(he);
fail:
	log_error("Failed to read the hall_effect sensor!");
still:
	add_to_results(he, 0);
	return get_avg_result(he);
}

void hall_effect_cleanup(sensor_halleffect_t *he) {
	gpio_line_close(he->pole);
	free(he->pole);
	free(he);
	log_info("Cleaned up hall_effect sensor");
}


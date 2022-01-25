#include "hall_effect.h"

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

double hall_effect_read(sensor_halleffect_t *he) {
	struct timespec start_spec, end_spec;
	enum gpio_val val;

	/* Wait until low state */
	do {
		if(gpio_line_read(he->pole, &val) != ERR_OK)
			goto fail;
	} while (val == GPIO_VAL_HIGH);
	
	/* Wait for high state to start */
	do {
		if(gpio_line_read(he->pole, &val) != ERR_OK)
			goto fail;
	} while (val == GPIO_VAL_LOW);
	clock_gettime(_POSIX_MONOTONIC_CLOCK, &start_spec);

	/* Wait for high state to end */
	do {
		if(gpio_line_read(he->pole, &val) != ERR_OK)
			goto fail;
	} while (val == GPIO_VAL_HIGH);
	clock_gettime(_POSIX_MONOTONIC_CLOCK, &end_spec);
	
	/* Calculate frequency */
	double time = ((end_spec.tv_nsec - start_spec.tv_nsec)/NANO_SEC_TO_SEC);
	time *= 2; /* Get period -> get high state length * 2 for full wave_length */
	return 1.0/time; /* Freq = 1 / period */

fail:
	log_error("Failed to read the hall_effect sensor!");
	return 0.0f;
}

void hall_effect_cleanup(sensor_halleffect_t *he) {
	gpio_line_close(he->pole);
	free(he->pole);
	free(he);
	log_info("Cleaned up hall_effect sensor");
}


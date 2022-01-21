#include "sensor.h"

sensors_t sensors = {0, NULL};

int add_sensor(void *init, void *cleanup, unsigned int interval) {
	sensor_t *s = calloc(1, sizeof(sensor_t));
	s->init = init;
	s->cleanup = cleanup;
	s->interval = interval;
	s->reference = NULL;

	sensors->sensors = realloc(sizeof(sensor_t) * ++count);
	sensors->sensors[count - 1] = s;
}

int initialize_sensors() {

}

int cleanup_sensors() {

}


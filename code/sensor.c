#include "sensor.h"

sensors_t sensors = {0, NULL};
pthread_t *threads = NULL;

void *sensor_start(void *ptr) {
	sensor_t *sensor = (sensor_t *)ptr;
	double val = 0.0f;
	while (1) {
		val = (*sensor->read)(sensor->reference);
		printf("read value %f\n", val);
		usleep(sensor->interval);
	}
	return NULL;
}

int add_sensor(void *init, void **init_args, void *cleanup, void *read, unsigned int interval) {
	sensor_t *s = calloc(1, sizeof(sensor_t));
	s->init = init;
	s->cleanup = cleanup;
	s->interval = interval;
	s->init_args = init_args;
	s->read = read;
	s->reference = NULL;

	sensors.sensors = realloc(sensors.sensors, sizeof(sensor_t) * ++sensors.count);
	memcpy((void *)&sensors.sensors[sensors.count - 1], s, sizeof(sensor_t));
	return 0;
}

int initialize_sensors() {
	threads = (pthread_t *) malloc(sensors.count * sizeof(pthread_t));
	for (int i = 0; i < sensors.count; i++) {
		sensor_t *sense = &sensors.sensors[i];
		sense->reference = (*sense->init)(sense->init_args);
		if (pthread_create(&threads[i], NULL, *sensor_start, (void *) sense) != 0) {
			log_error("failed to start a thread");
		}
	}
	pthread_join(threads[0], NULL); /* DELETE ME */
	return 0;
}

int cleanup_sensors() {
	log_info("Cleaning up sensors");
	for (int i = 0; i < sensors.count; i++) {
		sensor_t *sense = &sensors.sensors[i];
		(sense->cleanup)();
		free(sense);
	}
	free(sensors.sensors);
	return 0;
}


#include "sensor.h"

sensors_t sensors = {0, NULL};
pthread_t *threads = NULL;

void *sensor_start(void *ptr) {
	printf("Now I am here!\n");
	sensor_t *sensor = (sensor_t *)ptr;
	double d = 0.0f;
	while (1) {
		d = (*sensor->read)(sensor->reference);
		printf("read value %f\n", d);
		sleep(1);
	}
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
}

int initialize_sensors() {
	log_info("Preparing sensors");
	threads = malloc(sensors.count * sizeof(pthread_t));
	for (int i = 0; i < sensors.count; i++) {
		sensor_t *sense = &sensors.sensors[i];
		sense->reference = (*sense->init)(sense->init_args);
		//pthread_create(&(threads[i]), NULL, sensor_start, NULL);
	}
	sensor_start((void *)&sensors.sensors[0]);
}

int cleanup_sensors() {
	log_info("Cleaning up sensors");
	for (int i = 0; i < sensors.count; i++) {
		sensor_t *sense = &sensors.sensors[i];
		(sense->cleanup)();
		free(sense);
	}
	free(sensors.sensors);
}


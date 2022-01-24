#include "sensor.h"

sensors_t sensors = {0, NULL};
pthread_t *threads = NULL;

void signal_handler_child(int sig) {
	/* Do here anything a thread must */
	exit(0);
}

/**
 * @brief register the signal
 */
void register_signal_handler(void (*hndlr)(int)) {
    struct sigaction *sa = (struct sigaction *)malloc(sizeof(struct sigaction));
    sa->sa_handler = hndlr;
    sigfillset(&sa->sa_mask);
    sigaction(SIGINT, sa, NULL);
    sigaction(SIGHUP, sa, NULL);
    sigaction(SIGTERM, sa, NULL);
}

void *sensor_start(void *ptr) {
	sensor_t *sensor = (sensor_t *)ptr;
	while (1) {
		*sensor->result = (*sensor->read)(sensor->reference); /* TODO: Add mutex */
		printf("read value %f\n", *sensor->result);
		usleep(sensor->interval);
	}
	return NULL;
}

int add_sensor(void *init, void **init_args, void *cleanup, void *read, unsigned int interval, double *result) {
	sensor_t *s = (sensor_t *)malloc(sizeof(sensor_t));
	s->init = init;
	s->cleanup = cleanup;
	s->interval = interval;
	s->init_args = init_args;
	s->read = read;
	s->reference = NULL;
	s->result = result;

	sensors.sensors = realloc(sensors.sensors, sizeof(sensor_t) * ++sensors.count);
	memcpy((void *)&sensors.sensors[sensors.count - 1], s, sizeof(sensor_t));

	/* Register the signal handler */
	register_signal_handler(&signal_handler_child);

	return 0;
}

int initialize_sensors(void) {
	threads = (pthread_t *) malloc(sensors.count * sizeof(pthread_t));
	for (int i = 0; i < sensors.count; i++) {
		sensor_t *sense = &sensors.sensors[i];
		sense->reference = (*sense->init)(sense->init_args);
		if (pthread_create(&threads[i], NULL, *sensor_start, (void *) sense) != 0) {
			log_error("Failed to start a thread with ID %d \n", i);
		}
	}
	return 0;
}

int cleanup_sensors(void) {
	log_info("Cleaning up sensors");
	for (int i = 0; i < sensors.count; i++) {
		sensor_t *sense = &sensors.sensors[i];
		if (pthread_kill(threads[i], SIGKILL) != 0)
			log_error("Failed to kill signal for thread %d", i);
		(sense->cleanup)(sense->reference);
	}
	free(sensors.sensors);
	return 0;
}


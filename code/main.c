#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "tco_shmem.h"
#include "tco_libd.h"
#include "ultrasound.h"
#include "sensor.h"
#include "hall_effect.h"

#define UPDATE_RATE (1000000/30.0f) /* Seconds to wait before writing new sensor data to shmem */ 
#define SENSOR_COUNT 2 /* Amount of sensors in use */

int log_level = LOG_INFO | LOG_DEBUG | LOG_ERROR;
struct tco_shmem_data_sensor *shmem_sensor_data;
sem_t *shmem_sensor_sem;
/**
 * @brief Handler for signals. This ensures that deadlocks in shmems do not occur and  when
 * clontrold is closed
 * @param sig Signal number. This is ignored since this handler is registered for the right signals already.
 */
void handle_signals_master(int sig)
{
	cleanup_sensors();
    if (shmem_sensor_sem && sem_post(shmem_sensor_sem) == -1)
        log_error("sem_post: %s", strerror(errno));
    exit(0);
}

int main(int argc, const char *argv[]) {
	register_signal_handler(&handle_signals_master);
    if (log_init("sensord", "./log.txt") != 0)
    {
        printf("Failed to initialize the logger\n");
        return EXIT_FAILURE;
    }

    if (shmem_map(TCO_SHMEM_NAME_SENSOR, TCO_SHMEM_SIZE_SENSOR, TCO_SHMEM_NAME_SEM_SENSOR, O_RDWR, (void **)&shmem_sensor_data, &shmem_sensor_sem) != 0)
    {
        log_error("Failed to map shared memory and associated semaphore for sensor data");
        return EXIT_FAILURE;
    }

	/* Add sensor definitions here */
	double values[SENSOR_COUNT] = {0};
	pthread_mutex_t *locks[SENSOR_COUNT];
	
	void *us_1 = malloc(2 * sizeof(int));
	us_1 = (int[2]) {73, 138}; /* trig pin 73, echo pin 138 */
	locks[0] = add_sensor(us_init, (void **) &us_1, us_cleanup, us_get_distance, 200000, &(values[0])); /* 5 times a second */

	void *he_1 = malloc(sizeof(int)); 
	he_1 = (int[1]) {6}; /* pole is connect to pole 6 */
	locks[1] = add_sensor(hall_effect_init, (void **) &he_1, hall_effect_cleanup, hall_effect_read, 1000000.0/60.0f, &(values[1]));
	/* End sensor definition */
	initialize_sensors();
	
	/* Write all values to shmem */
	double values_copy[SENSOR_COUNT] = {0};
	while (1) {
		for (int i = 0; i < SENSOR_COUNT; i++)
		{
			pthread_mutex_lock(locks[i]); /* Read latest value */
			memcpy(&values_copy[i], &values[i], sizeof(double));
			pthread_mutex_unlock(locks[i]);
		}
		sem_wait(shmem_sensor_sem);
		/* Enter Critical Section */
		shmem_sensor_data->ultrasound_left = values_copy[0];
		shmem_sensor_data->hall_effect_rpm = values_copy[1];
		/* Exit Critical Section */
		sem_post(shmem_sensor_sem);
		usleep(UPDATE_RATE);
	}

    return 0;
}

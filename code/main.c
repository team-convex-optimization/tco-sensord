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

#define ULTRASOUND_TRIGGER 73 /* GPIO 16 */
#define ULTRASOUND_ECHO 138 /* GPIO 18 */
#define MIN_DRIVE_CLEARANCE 50.0f /* Minimum clearance the US sensor must read to not raise emergency flag */

int log_level = LOG_INFO | LOG_DEBUG | LOG_ERROR;
struct tco_shmem_data_plan *plan_data;
sem_t *plan_data_sem;

/**
 * @brief Handler for signals. This ensures that deadlocks in shmems do not occur and  when
 * clontrold is closed
 * @param sig Signal number. This is ignored since this handler is registered for the right signals already.
 */
static void handle_signals_master(int sig)
{
	cleanup_sensors();
    if (plan_data_sem && sem_post(plan_data_sem) == -1)
        log_error("sem_post: %s", strerror(errno));
    exit(0);
}

int main(int argc, const char *argv[]) {
	register_signal_handler(handle_signals_master);
    if (log_init("sensord", "./log.txt") != 0)
    {
        printf("Failed to initialize the logger\n");
        return EXIT_FAILURE;
    }

    if (shmem_map(TCO_SHMEM_NAME_PLAN, TCO_SHMEM_SIZE_PLAN, TCO_SHMEM_NAME_SEM_PLAN, O_RDWR, (void **)&plan_data, &plan_data_sem) != 0)
    {
        log_error("Failed to map shared memory and associated semaphore");
        return EXIT_FAILURE;
    }
	
	/* Add sensor definitions here */
	void *us_1 = malloc(2 * sizeof(int));
	us_1 = (int[2]) {73, 138}; /* trig pin 73, echo pin 138 */
	add_sensor(us_init, &us_1, us_cleanup, us_get_distance, 200000); /* 5 times a second */
	
	/* End sensor defintion */
	initialize_sensors();
	
	/* Wait for the first thread. They either all run and there must be at least 1 sensor for this program to be running with any sense */

	//pthread_join(threads[0], NULL); /* DELETE ME */
    return 0;
}

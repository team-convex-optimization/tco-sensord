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
sensor_ultrasound *us;
struct tco_shmem_data_control *control_data;
sem_t *control_data_sem;

/**
 * @brief Handler for signals. This ensures that deadlocks in shmems do not occur and  when
 * clontrold is closed
 * @param sig Signal number. This is ignored since this handler is registered for the right signals already.
 */
static void handle_signals(int sig)
{
    if (sem_post(control_data_sem) == -1)
        log_error("sem_post: %s", strerror(errno));
    exit(0);
}

int main(int argc, const char *argv[]) {
    struct sigaction sa;
    sa.sa_handler = handle_signals;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if (log_init("sensord", "./log.txt") != 0)
    {
        printf("Failed to initialize the logger\n");
        return EXIT_FAILURE;
    }

    if (shmem_map(TCO_SHMEM_NAME_CONTROL, TCO_SHMEM_SIZE_CONTROL, TCO_SHMEM_NAME_SEM_CONTROL, O_RDWR, (void **)&control_data, &control_data_sem) != 0)
    {
        log_error("Failed to map shared memory and associated semaphore");
        return EXIT_FAILURE;
    }

	void *us_init_1 = malloc(2 * sizeof(int));
	us_init_1 = (int[2]) {ULTRASOUND_TRIGGER, ULTRASOUND_ECHO};
	add_sensor(us_init, &us_init_1, us_cleanup, us_get_distance, 1);
	initialize_sensors();

    return 0;
}

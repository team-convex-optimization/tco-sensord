#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#include "tco_shmem.h"
#include "tco_libd.h"
#include "ultrasound.h"

#define ULTRASOUND_TRIGGER 18
#define ULTRASOUND_ECHO 16
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
    if (us)
        free(us);
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
/*
    if (shmem_map(TCO_SHMEM_NAME_CONTROL, TCO_SHMEM_SIZE_CONTROL, TCO_SHMEM_NAME_SEM_CONTROL, O_WRONLY, (void **)&control_data, &control_data_sem) != 0)
    {
        log_error("Failed to map shared memory and associated semaphore");
        return EXIT_FAILURE;
    }
*/
    us = us_init(ULTRASOUND_TRIGGER, ULTRASOUND_ECHO);
    assert(us != NULL);

    while (1) {
        if (us_get_distance(us) < MIN_DRIVE_CLEARANCE) { /* XXX : As more reads are read, combine sem_wait into one call */
            sem_wait(control_data_sem);
            control_data->emergency = 1;
            sem_post(control_data_sem);
        } else {
            sem_wait(control_data_sem);
            control_data->emergency = 0;
            sem_post(control_data_sem);
        }
    }


    return 0;
}

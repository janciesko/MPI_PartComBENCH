#ifndef __PARTIX_H__
#define __PARTIX_H__

#define NUM_THREADS_DEFAULT 8
#define NUM_TASKS_DEFAULT NUM_THREADS_DEFAULT
#define PARTITIONS_DEFAULT 8
#define PARTLENGTH_DEFAULT 16

#include <types.h>
#include <thread.h>

void partix_init(int argc, char *argv[], partix_config_t *conf) {
  conf->num_tasks = argc > 1 ? atoi(argv[1]) : NUM_TASKS_DEFAULT;
  conf->num_threads = argc > 2 ? atoi(argv[2]) : NUM_THREADS_DEFAULT;
  conf->num_partitions = argc > 3 ? atoi(argv[3]) : PARTITIONS_DEFAULT;
  conf->num_partlength = argc > 4 ? atoi(argv[4]) : PARTLENGTH_DEFAULT;
}

enum Options {
  partix_noise_on,
  partix_noise_off,
};

void partix_add_noise() {}

#endif /* __PARTIX_H__*/
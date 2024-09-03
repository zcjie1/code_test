#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include <sys/stat.h>

#include <rte_cycles.h>
#include <rte_alarm.h>

#include "type.h"

int generate_logdir(char *dir_path) {
    struct stat info;

    if (stat(dir_path, &info) != 0) {
        if (errno == ENOENT) {
            if (mkdir(dir_path, 0666) != 0) {
                perror("Failed to create directory");
                return -1;
            }
        } else {
            perror("Failed to check directory");
            return -1;
        }
    } else if (!S_ISDIR(info.st_mode)) {
        fprintf(stderr, "Path '%s' exists but is not a directory.\n", dir_path);
        return -1;
    }

    return 0;
}

void show_stats(void *param)
{
    struct status *statistics = (struct status*)param;
    FILE *log = statistics->log;

	time_t now;
    struct tm *local_time;
    char timestamp[26];

	uint64_t total = statistics->total_rx + statistics->total_tx;
	uint64_t real = statistics->real_rx + statistics->real_tx;
	uint64_t drop = total - real;

	// 获取当前时间
    time(&now);
    local_time = localtime(&now);

	// 格式化时间戳
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local_time);

	fprintf(log, "\n==================Packets statistics=======================");
	fprintf(log, "\nPackets sent: %24"PRIu64
			"\nPackets received: %20"PRIu64
			"\nPackets dropped: %21"PRIu64,
			statistics->real_tx,
			statistics->real_rx,
			drop);
	fprintf(log, "\n===================%s=====================\n\n", timestamp);
	fflush(stdout);

	rte_eal_alarm_set(PERIOD * US_PER_S, show_stats, statistics);
}
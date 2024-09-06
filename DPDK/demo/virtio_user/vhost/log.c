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

void period_show_stats(void *param)
{
    struct status *statistics = (struct status *)param;
    FILE *log = statistics->log;

	time_t now;
    struct tm *local_time;
    char timestamp[26];

    uint64_t rx_drop = statistics->total_rx_num - statistics->real_rx_num;
    uint64_t tx_drop = statistics->total_tx_num - statistics->real_tx_num;
	uint64_t total_drop = rx_drop + tx_drop;

	// 获取当前时间
    time(&now);
    local_time = localtime(&now);

	// 格式化时间戳
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local_time);

	fprintf(log, "\n==================Packets statistics=======================");
	fprintf(log, "\nPackets sent: %24"PRIu64
			"\nPackets received: %20"PRIu64
			"\nPackets dropped: %21"PRIu64
            "\nPackets TX dropped: %18"PRIu64
            "\nPackets RX dropped: %18"PRIu64,
			statistics->real_tx_num,
			statistics->real_rx_num,
			total_drop,
            tx_drop,
            rx_drop);
	fprintf(log, "\n===================%s=====================\n\n", timestamp);
	fflush(log);

	rte_eal_alarm_set(PERIOD * US_PER_S, period_show_stats, statistics);
}

void show_results(void *param)
{
    struct status *s = (struct status *)param;
    FILE *log = s->log;
    char time_buffer[30];
    double rx_rate = 0.0;
    double tx_rate = 0.0;
    
    clock_gettime(CLOCK_MONOTONIC, &s->end_time);
    long seconds = s->end_time.tv_sec - s->start_time.tv_sec;
    long nanoseconds = s->end_time.tv_nsec - s->start_time.tv_nsec;
    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += NS_PER_S;
    }
    snprintf(time_buffer, 30, "%ld.%09ld", seconds, nanoseconds);
    double real_time = seconds + nanoseconds / (double)NS_PER_S;
    
    uint64_t rx_drop = s->total_rx_num - s->real_rx_num;
    uint64_t tx_drop = s->total_tx_num - s->real_tx_num;
	uint64_t total_drop = rx_drop + tx_drop;

    rx_rate = (double)s->rx_bytes / real_time / 1000 / 1000;
    tx_rate = (double)s->tx_bytes / real_time / 1000 / 1000;

    fprintf(log, "\n==================Packets statistics=======================");
	fprintf(log,
            "\nDuration Time: %22ss"
			"\nPackets received: %20"PRIu64
            "\nBytes received: %22"PRIu64
            "\nReceive Rate: %20.2fMB/s"
            "\nPackets sent: %24"PRIu64
            "\nBytes sent: %26"PRIu64
            "\nSend Rate: %23.2fMB/s"
			"\nPackets dropped: %21"PRIu64
            "\nPackets RX dropped: %18"PRIu64
            "\nPackets TX dropped: %18"PRIu64,
            time_buffer,
			s->real_rx_num,
            s->rx_bytes,
            rx_rate,
            s->real_tx_num,
            s->tx_bytes,
            tx_rate,
			total_drop,
            rx_drop,
            tx_drop);
	fprintf(log, "\n===========================================================\n\n");
	fflush(log);
}
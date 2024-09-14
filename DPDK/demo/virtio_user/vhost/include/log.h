#ifndef __LOG_H__
#define __LOG_H__

#include "type.h"

#define LOG_DIR "log"
#define LOG_FILE "log/vhost_statistic.log"

#define PERIOD 5

void log_init(void);
void log_exit(void);

#endif // !__LOG_H__
#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/un.h>
#include <poll.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <semaphore.h>

// colors
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_BOLD_ON   "\x1b[1m"
#define ANSI_BOLD_OFF   "\x1b[22m"

#define DEFAULT_SEM_PATH "./.semaphore"
#define DEFAULT_PATH "./"
#define MODULE_INDENTATION "  "
#define TEMP_PATH "/tmp/"
#define COMM_PORT 7246
#define BACKLOG 25

#define NO_ATTR NULL
#define NO_SHARE 0

#define ADDED_DIFF_EXEC "diff --changed-group-format=\"%>\" --unchanged-group-format=\"\" --old-group-format=\"\" --new-group-format=\"%>\""
#define REMOVED_DIFF_EXEC "diff --changed-group-format=\"%<\" --unchanged-group-format=\"\" --old-group-format=\"%<\" --new-group-format=\"\""

#define false 0
#define true 1

#define VERBOSE ((mode & 0x10) == 0x10)

// Global variables
short mode;

#endif
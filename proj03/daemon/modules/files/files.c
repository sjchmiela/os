#ifndef COMMON_H
#include "../../../common/common.h"
#endif
#include "files.h"
#include <sys/inotify.h>

short files_exited = false;
int inotify_fd = -1;
int idir_fd = -1;
char buffer[MODULE_BUFFER_SIZE] __attribute__ ((aligned(__alignof__(struct inotify_event))));
char message[MODULE_BUFFER_SIZE];

void files_init(char *path) {

  if((inotify_fd = inotify_init1(IN_NONBLOCK)) < 0) {
    printf(ANSI_COLOR_RED);
    printf("Could not init inotify.\n");
    printf("\t%s\n", strerror(errno));
    printf(ANSI_COLOR_RESET);
    exit(EXIT_FAILURE);
  }

  if((idir_fd = inotify_add_watch(inotify_fd, path, IN_ALL_EVENTS)) < 0) {
    printf(ANSI_COLOR_RED);
    printf("Could not add watch to specified directory.\n");
    printf("\t%s\n", strerror(errno));
    printf(ANSI_COLOR_RESET);
    exit(EXIT_FAILURE);
  }
}

void files_refresh(int fd) {
  struct inotify_event *event = NULL;
  char *pointer;
  int read_bytes = 0;
  int formatted = 0;
  while((read_bytes = read(inotify_fd, buffer, sizeof(buffer))) > 0) {
    
    for(pointer = buffer; pointer + sizeof(struct inotify_event) < buffer + read_bytes; pointer += sizeof(struct inotify_event) + event->len) {

      event = (struct inotify_event *)pointer;

      if(event->len == 0) {
        continue;
      }
      
      if(event->mask & IN_OPEN) {
        formatted = snprintf(message, MODULE_BUFFER_SIZE, MODULE_INDENTATION"["FIL_ABBR"] %s opened\n", event->name);
        write(fd, message, formatted);
      }

      if(event->mask & IN_ACCESS) {
        formatted = snprintf(message, MODULE_BUFFER_SIZE, MODULE_INDENTATION"["FIL_ABBR"] %s accessed\n", event->name);
        write(fd, message, formatted);
      }

      if(event->mask & IN_MODIFY) {
        formatted = snprintf(message, MODULE_BUFFER_SIZE, MODULE_INDENTATION"["FIL_ABBR"] %s modified\n", event->name);
        write(fd, message, formatted);
      }

      if(event->mask & IN_DELETE) {
        formatted = snprintf(message, MODULE_BUFFER_SIZE, MODULE_INDENTATION"["FIL_ABBR"] %s deleted\n", event->name);
        write(fd, message, formatted);
      }

      if(event->mask & IN_ATTRIB) {
        formatted = snprintf(message, MODULE_BUFFER_SIZE, MODULE_INDENTATION"["FIL_ABBR"] %ss metadata changed\n", event->name);
        write(fd, message, formatted);
      }

      if(event->mask & (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE)) {
        formatted = snprintf(message, MODULE_BUFFER_SIZE, MODULE_INDENTATION"["FIL_ABBR"] %s closed\n", event->name);
        write(fd, message, formatted);
      }

    }
  }

  if(read_bytes < 0 && errno != EAGAIN) {
    printf(ANSI_COLOR_RED);
    printf("Could not read from inotify_fd.\n");
    printf("\t%s\n", strerror(errno));
    printf(ANSI_COLOR_RESET);
    exit(EXIT_FAILURE);
  }
}

void files_exit() {
  if(files_exited == false) {
    files_exited = true;
    if(VERBOSE) {
      printf(ANSI_COLOR_CYAN);
      printf("Closing inotify.\n");
      printf(ANSI_COLOR_RESET);
    }
    close(inotify_fd);
  }
}
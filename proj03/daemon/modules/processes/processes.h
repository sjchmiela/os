#ifndef PROCESSES_H
#define PROCESSES_H

#define PRC_ABBR "PRC"
#define PROCESSES_EXEC "ps -e -o comm | sort | uniq | grep -v COMM"
#define MODULE_BUFFER_SIZE 512
#define PRC_DIFF_EXEC_SUFFIX " | sed -e 's/^/"MODULE_INDENTATION"["PRC_ABBR"] "MODULE_INDENTATION"/'"

#define PROC_ADDED_MESSAGE MODULE_INDENTATION"["PRC_ABBR"] Started processes:\n"
#define PROC_REMOVED_MESSAGE MODULE_INDENTATION"["PRC_ABBR"] Ended processes:\n"

void processes_init();
void processes_refresh(int fd);
void processes_exit();

#endif
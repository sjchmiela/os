#ifndef MEMORY_H
#define MEMORY_H

#define MEM_ABBR "MEM"
#define MEMORY_FILE "/proc/meminfo"
#define MODULE_BUFFER_SIZE 512
#define DIFF_EXEC "diff --changed-group-format=\"%>\" --unchanged-group-format=\"\" --old-group-format=\"\" --new-group-format=\"\""
#define DIFF_EXEC_SUFFIX " | sed -e 's/^/"MODULE_INDENTATION"["MEM_ABBR"] /' | grep MemFree"

void memory_init();
void memory_refresh(int fd);
void memory_exit();

#endif
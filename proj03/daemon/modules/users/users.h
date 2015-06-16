#ifndef USERS_H
#define USERS_H

#define USR_ABBR "USR"
#define USERS_EXEC "who | cut -d' ' -f1,3 | sort | uniq"
#define MODULE_BUFFER_SIZE 512
#define USR_DIFF_EXEC_SUFFIX " | sed -e 's/^/"MODULE_INDENTATION"["USR_ABBR"] "MODULE_INDENTATION"/'"

#define ADDED_MESSAGE MODULE_INDENTATION"["USR_ABBR"] Added users:\n"
#define REMOVED_MESSAGE MODULE_INDENTATION"["USR_ABBR"] Removed users:\n"

void users_init();
void users_refresh(int fd);
void users_exit();

#endif
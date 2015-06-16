#ifndef FILES_H
#define FILES_H

#define FIL_ABBR "FIL"
#define MODULE_BUFFER_SIZE 512

void files_init(char *path);
void files_refresh(int fd);
void files_exit();

#endif
#ifndef COMMON_H
#include "../../../common/common.h"
#endif
#include "memory.h"

#define FILENAME_LEN (sizeof(TEMP_PATH)+sizeof("XXXXXX"))

short memory_exited = false;
char old_memory_filename[FILENAME_LEN];
char new_memory_filename[FILENAME_LEN];
char command[sizeof(DIFF_EXEC)+1+FILENAME_LEN+1+FILENAME_LEN+sizeof(DIFF_EXEC_SUFFIX)];
char buffer[MODULE_BUFFER_SIZE];

void memory_update_to_fd(int target_fd);
void memory_prepare_filename(char filename[FILENAME_LEN]);
void memory_exit();

void memory_init() {
  memory_prepare_filename(old_memory_filename);

  int fd = -1;

  if((fd = mkstemp(old_memory_filename)) < 0) {
    printf(ANSI_COLOR_RED);
    printf("Could not open temporary file for memory storage.\n");
    printf("\t%s\n", strerror(errno));
    printf(ANSI_COLOR_RESET);
    exit(EXIT_FAILURE);
  }

  if(VERBOSE) {
    printf(ANSI_COLOR_GREEN);
    printf("Memory guard created temp file: '%s'.\n", old_memory_filename);
    printf(ANSI_COLOR_RESET);
  }

  memory_update_to_fd(fd);

  close(fd);
}

void memory_prepare_filename(char filename[FILENAME_LEN]) {
  strcpy(filename, TEMP_PATH);
  strcat(filename, "XXXXXX");
}

void memory_prepare_command() {
  strcpy(command, DIFF_EXEC);
  strcat(command, " ");
  strcat(command, old_memory_filename);
  strcat(command, " ");
  strcat(command, new_memory_filename);
  strcat(command, DIFF_EXEC_SUFFIX);
}

void memory_update_to_fd(int target_fd) {
  int fd = open(MEMORY_FILE, O_RDONLY);
  int read_bytes = 0;
  while((read_bytes = read(fd, buffer, MODULE_BUFFER_SIZE)) > 0) {
    if(write(target_fd, buffer, read_bytes) < read_bytes) {
      printf(ANSI_COLOR_RED);
      printf("Could not write whole buffer to memory storage.\n");
      printf("\t%s\n", strerror(errno));
      printf(ANSI_COLOR_RESET);
    }
  }
  close(fd);
}

void memory_refresh(int fd) {
  int new_fd = -1;

  memory_prepare_filename(new_memory_filename);

  if((new_fd = mkstemp(new_memory_filename)) < 0) {
    printf(ANSI_COLOR_RED);
    printf("Could not open temporary file for memory update.\n");
    printf("\t%s\n", strerror(errno));
    printf(ANSI_COLOR_RESET);
    exit(EXIT_FAILURE);
  }

  memory_update_to_fd(new_fd);

  close(new_fd);
  
  memory_prepare_command();

  // if(VERBOSE) {
  //   printf(ANSI_COLOR_GREEN);
  //   printf("Running: '%s'.\n", command);
  //   printf(ANSI_COLOR_RESET);
  // }

  FILE *diff = NULL;
  if((diff = popen(command, "r")) == NULL) {
    printf(ANSI_COLOR_RED);
    printf("Could not popen diff.\n");
    printf("\t%s\n", strerror(errno));
    printf(ANSI_COLOR_RESET);
    exit(EXIT_FAILURE);
  }

  int read_bytes = 0;

  while((read_bytes = fread(buffer, sizeof(char), MODULE_BUFFER_SIZE, diff)) > 0) {
    write(fd, buffer, read_bytes);
  }

  fclose(diff);

  unlink(old_memory_filename);
  strcpy(old_memory_filename, new_memory_filename);
}

void memory_exit() {
  if(memory_exited == true) { return; }
  if(VERBOSE) {
    printf(ANSI_COLOR_CYAN);
    printf("Removing old memory guard file: '%s'.\n", old_memory_filename);
    printf(ANSI_COLOR_RESET);
  }
  memory_exited = true;
  unlink(old_memory_filename);
}
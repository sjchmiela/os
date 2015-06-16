#ifndef COMMON_H
#include "../../../common/common.h"
#endif
#include "processes.h"

#define FILENAME_LEN (sizeof(TEMP_PATH)+sizeof("XXXXXX"))

short processes_exited = false;
char old_processes_filename[FILENAME_LEN];
char new_processes_filename[FILENAME_LEN];
char added_command[sizeof(ADDED_DIFF_EXEC)+1+FILENAME_LEN+1+FILENAME_LEN+sizeof(PRC_DIFF_EXEC_SUFFIX)];
char removed_command[sizeof(REMOVED_DIFF_EXEC)+1+FILENAME_LEN+1+FILENAME_LEN+sizeof(PRC_DIFF_EXEC_SUFFIX)];
char buffer[MODULE_BUFFER_SIZE];

void processes_update_to_fd(int target_fd);
void processes_prepare_filename(char filename[FILENAME_LEN]);

void processes_init() {
  processes_prepare_filename(old_processes_filename);

  int fd = -1;

  if((fd = mkstemp(old_processes_filename)) < 0) {
    printf(ANSI_COLOR_RED);
    printf("Could not open temporary file for processes storage.\n");
    printf("\t%s\n", strerror(errno));
    printf(ANSI_COLOR_RESET);
    exit(EXIT_FAILURE);
  }

  if(VERBOSE) {
    printf(ANSI_COLOR_GREEN);
    printf("processes guard created temp file: '%s'.\n", old_processes_filename);
    printf(ANSI_COLOR_RESET);
  }

  processes_update_to_fd(fd);

  close(fd);
}

void processes_prepare_filename(char filename[FILENAME_LEN]) {
  strcpy(filename, TEMP_PATH);
  strcat(filename, "XXXXXX");
}

void processes_prepare_commands() {
  strcpy(added_command, ADDED_DIFF_EXEC);
  strcat(added_command, " ");
  strcat(added_command, old_processes_filename);
  strcat(added_command, " ");
  strcat(added_command, new_processes_filename);
  strcat(added_command, PRC_DIFF_EXEC_SUFFIX);

  strcpy(removed_command, REMOVED_DIFF_EXEC);
  strcat(removed_command, " ");
  strcat(removed_command, old_processes_filename);
  strcat(removed_command, " ");
  strcat(removed_command, new_processes_filename);
  strcat(removed_command, PRC_DIFF_EXEC_SUFFIX);
}

void processes_update_to_fd(int target_fd) {
  FILE *file = popen(PROCESSES_EXEC, "r");
  int read_bytes = 0;
  while((read_bytes = fread(buffer, sizeof(char), MODULE_BUFFER_SIZE, file)) > 0) {
    if(write(target_fd, buffer, read_bytes) < read_bytes) {
      printf(ANSI_COLOR_RED);
      printf("Could not write whole buffer to processes storage.\n");
      printf("\t%s\n", strerror(errno));
      printf(ANSI_COLOR_RESET);
    }
  }
  fclose(file);
}

void proc_diff_with(char *message, char *command, int fd) {
  // if(VERBOSE) {
  //   printf(ANSI_COLOR_GREEN);
  //   printf("Running: '%s'.\n", command);
  //   printf(ANSI_COLOR_RESET);
  // }

  FILE *diff;

  diff = NULL;
  if((diff = popen(command, "r")) == NULL) {
    printf(ANSI_COLOR_RED);
    printf("Could not popen diff.\n");
    printf("\t%s\n", strerror(errno));
    printf(ANSI_COLOR_RESET);
    exit(EXIT_FAILURE);
  }

  int read_bytes = 0;
  short printed_message = false;

  while((read_bytes = fread(buffer, sizeof(char), MODULE_BUFFER_SIZE, diff)) > 0) {
    if(printed_message == false) {
      write(fd, message, strlen(message));
      printed_message = true;
    }
    write(fd, buffer, read_bytes);
  }

  fclose(diff);
}

void processes_refresh(int fd) {
  int new_fd = -1;

  processes_prepare_filename(new_processes_filename);

  if((new_fd = mkstemp(new_processes_filename)) < 0) {
    printf(ANSI_COLOR_RED);
    printf("Could not open temporary file for processes update.\n");
    printf("\t%s\n", strerror(errno));
    printf(ANSI_COLOR_RESET);
    exit(EXIT_FAILURE);
  }

  processes_update_to_fd(new_fd);

  close(new_fd);
  
  processes_prepare_commands();

  proc_diff_with(PROC_ADDED_MESSAGE, added_command, fd);
  proc_diff_with(PROC_REMOVED_MESSAGE, removed_command, fd);

  unlink(old_processes_filename);
  strcpy(old_processes_filename, new_processes_filename);
}

void processes_exit() {
  if(processes_exited == true) { return; }
  if(VERBOSE) {
    printf(ANSI_COLOR_CYAN);
    printf("Removing old processes guard file: '%s'.\n", old_processes_filename);
    printf(ANSI_COLOR_RESET);
  }
  processes_exited = true;
  unlink(old_processes_filename);
}
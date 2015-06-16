#include "../common/common.h"
#include "modules/modules.h"
#define WELCOME_MSG_LENGTH 128
#define MAX_NAME 16

short path_to_watch_alloced = false;
char *path_to_watch = NULL;
struct sockaddr_in socket_remote;
char *name = NULL;
char welcome_message[WELCOME_MSG_LENGTH];
time_t current_time;

void printHelp();
void cleanup();
void nonblock_socket(int fd);
void reuse_socket(int fd);
void sigint_handler(int signum);
void print_ip();

int main(int argc, char *const argv[]) {
  mode = 0;
  // Parse command line arguments
  opterr = 0;
  int c;

  while((c = getopt(argc, argv, "hvd:n:")) != -1) {
    switch(c) {
      case 'h':
        printHelp();
        return 0;
      case 'v':
        mode = mode | 0x10;
        break;
      case 'd':
        path_to_watch = optarg;
        break;
      case 'n':
        name = optarg;
        break;
      case '?':
        printf(ANSI_COLOR_RED);
        if(optopt == 'd') {
          printf("Option -d requires directory to watch.\n");
        } else {
          printf("Unknown option character '\\x%x'.\n", optopt);
        }
        printf(ANSI_COLOR_RESET);
      default:
        return -1;
    }
  }

  if(path_to_watch == NULL) {
    path_to_watch_alloced = true;
    path_to_watch = malloc(strlen(DEFAULT_PATH)+1);
    strcpy(path_to_watch, DEFAULT_PATH);
  }

  atexit(cleanup);
  signal(SIGINT, sigint_handler);

  if(VERBOSE) {
    printf(ANSI_COLOR_GREEN);
    printf("Directory to watch: '%s'.\n", path_to_watch);
    printf(ANSI_COLOR_RESET);
  }

  if(name != NULL && strlen(name) > MAX_NAME) {
    name[MAX_NAME-1] = '\0';
  }

  if(VERBOSE && name != NULL) {
    printf(ANSI_COLOR_GREEN);
    printf("Name for computer: '%s'.\n", name);
    printf(ANSI_COLOR_RESET);
  }

  // file changes
  // inotify_init
  files_init(path_to_watch);

  // memory check
  // `cat /proc/meminfo`
  memory_init();

  // logged in users
  // `who | cut -d' ' -f1 | sort | uniq`
  users_init();

  // running processes
  processes_init();

  int remote_socket = -1;

  if((remote_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    printf(ANSI_COLOR_RED);
    printf("Error creating IP socket.\n");
    printf(ANSI_COLOR_RESET);
    exit(EXIT_FAILURE);
  }

  reuse_socket(remote_socket);

  memset(&socket_remote, 0, sizeof socket_remote);
  socket_remote.sin_family = AF_INET;
  socket_remote.sin_addr.s_addr = htonl(INADDR_ANY);
  socket_remote.sin_port = htons(COMM_PORT);

  if(bind(remote_socket, (struct sockaddr *)&socket_remote, sizeof(socket_remote)) < 0) {
    printf(ANSI_COLOR_RED);
    printf("Error binding IP socket.\n");
    printf("\t%s\n", strerror(errno));
    printf(ANSI_COLOR_RESET);
    exit(EXIT_FAILURE);
  }

  listen(remote_socket, BACKLOG);

  int console_socket = -1;
  struct tm *tm_p;
  int formatted = 0;
  socklen_t address_length = sizeof(socket_remote);

  print_ip();

  while((console_socket = accept(remote_socket, NULL, NULL)) >= 0) {
    if(VERBOSE) {
      if(getpeername(console_socket, (struct sockaddr *)&socket_remote, &address_length) < 0) {
        printf(ANSI_COLOR_RED);
        printf("Error getting client's IP address.\n");
        printf("\t%s\n", strerror(errno));
        printf(ANSI_COLOR_RESET);
      } else {
        printf(ANSI_COLOR_GREEN);
        printf("New connection from %s, sending report...", inet_ntoa(socket_remote.sin_addr));
        printf(ANSI_COLOR_RESET);
      }
    }

    if(getsockname(console_socket, (struct sockaddr *)&socket_remote, &address_length) < 0) {
      printf(ANSI_COLOR_RED);
      printf("Error getting current IP address.\n");
      printf("\t%s\n", strerror(errno));
      printf(ANSI_COLOR_RESET);
    }

    current_time = time(NULL);
    tm_p = localtime(&current_time);

    if(name == NULL) {
      formatted = snprintf(welcome_message, WELCOME_MSG_LENGTH, "Report for "ANSI_BOLD_ON"%s"ANSI_BOLD_OFF" @ %.2d:%.2d:%.2d\n", inet_ntoa(socket_remote.sin_addr), tm_p->tm_hour, tm_p->tm_min, tm_p->tm_sec);
    } else {
      formatted = snprintf(welcome_message, WELCOME_MSG_LENGTH, "Report for "ANSI_BOLD_ON"%s"ANSI_BOLD_OFF" (%s) @ %.2d:%.2d:%.2d\n", name, inet_ntoa(socket_remote.sin_addr), tm_p->tm_hour, tm_p->tm_min, tm_p->tm_sec);
    }
    
    

    write(console_socket, welcome_message, formatted);
    memory_refresh(console_socket);
    users_refresh(console_socket);
    processes_refresh(console_socket);
    files_refresh(console_socket);
    close(console_socket);

    if(VERBOSE) {
      printf(" sent!\n");
    }
  }

  return 0;
}

void printHelp() {
  printf("\nUsage: daemon [-v] [-d <directory>]\n"
"\n"
"  Options:\n"
"\n"
"    -h             output usage information\n"
"    -d <path>      path to watch\n"
"    -v             verbose mode\n\n");
}

void sigint_handler(int signum) {
  cleanup();
  exit(0);
}

void cleanup() {
  if(path_to_watch_alloced == true && path_to_watch != NULL) {
    if(VERBOSE) {
      printf(ANSI_COLOR_CYAN); 
      printf("Freeing path to watch...\n");
      printf(ANSI_COLOR_RESET);
    }
    unlink(path_to_watch); 
    path_to_watch = NULL;
  }
  memory_exit();
  users_exit();
  processes_exit();
  files_exit();
}

void reuse_socket(int fd) {
  int on = 1;
  if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
    printf(ANSI_COLOR_RED);
    printf("Error setting address reusing.\n");
    printf("\t%s\n", strerror(errno));
    printf(ANSI_COLOR_RESET);
    exit(EXIT_FAILURE);
  }
}

void print_ip() {
  int fd;
  struct ifreq ifr;
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, "eth1", IFNAMSIZ-1);
  ioctl(fd, SIOCGIFADDR, &ifr);
  close(fd);
  printf(ANSI_COLOR_GREEN"Current IP of daemon: %s\n"ANSI_COLOR_RESET, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
}
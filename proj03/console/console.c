#include "../common/common.h"

#define MAX_IP_LEN 15
#define MAX_LINE_LEN 128
#define ADD_REMOTE_MSG "/add "
#define LIST_MSG "/list"
#define REFRESH_MSG "/refresh"
#define STOP_MSG "/stop"
#define START_MSG "/start"
#define REMOVE_MSG "/remove "
#define MAX_RMSG_LENGTH 128
#define HLINE_LENGTH 50

// Mode parsing
#define VERBOSE ((mode & 0x10) == 0x10)
#define LOCAL ((mode & 0x01) == 0x01)
#define REMOTE ((mode & 0x02) == 0x02)

// Global variables
char ip[15];
char message_received[MAX_RMSG_LENGTH];
pthread_t keyboard_thread;
pthread_t chat_thread;
char *command;
sem_t *refresh_sem = SEM_FAILED;
int refresh_interval = 10;
short refresh = true;

struct remote {
  char ip[MAX_IP_LEN];
  struct remote *next;
};

struct remote *remotes = NULL;

void sigint_handler(int signum);
void printHelp();
void* keyboard_do(void *argument);
void cleanup();
void add_remote(char *ip);
void list_remotes();
void refresh_remotes();
void sigalarm_handler(int signum);
void remove_remote(int id);

int main(int argc, char *const argv[]) {
  // Parse command line arguments
  opterr = 0;
  int c;
  mode = 0;

  while((c = getopt(argc, argv, "hvc:r:")) != -1) {
    switch(c) {
      case 'v':
        mode = mode | 0x10;
        break;
      case 'h':
        printHelp();
        return 0;
      case 'c':
        add_remote(optarg);
        break;
      case 'r':
        refresh_interval = atoi(optarg);
        break;
      case '?':
        printf(ANSI_COLOR_RED);
        if(optopt == 'c') {
          printf("Option -c requires IP.\n");
        } else if(isprint(optopt)) {
          printf("Unknown option '-%c'.\n", optopt);
        } else {
          printf("Unknown option character '\\x%x'.\n", optopt);
        }
        printf(ANSI_COLOR_RESET);
        return -1;
      default:
        return -1;
    }
  }

  command = malloc(MAX_LINE_LEN);

  atexit(cleanup);
  signal(SIGINT, sigint_handler);
  signal(SIGALRM, sigalarm_handler);

  sem_unlink(DEFAULT_SEM_PATH);

  if((refresh_sem = sem_open(DEFAULT_SEM_PATH, O_CREAT | O_EXCL, S_IRWXU, 1)) == SEM_FAILED) {
    printf(ANSI_COLOR_RED);
    printf("Could not initialize semaphore.\n");
    printf("%s\n", strerror(errno));
    printf(ANSI_COLOR_RESET);
    exit(EXIT_FAILURE);
  }

  if(VERBOSE) {
    printf(ANSI_COLOR_GREEN);
    printf("Refreshing every %d seconds.\n", refresh_interval);
    printf(ANSI_COLOR_RESET);
  }

  sigalarm_handler(SIGALRM);
  alarm(refresh_interval);

  chat_thread = pthread_self();

  if(pthread_create(&keyboard_thread, NO_ATTR, keyboard_do, NO_ATTR) < 0) {
    printf(ANSI_COLOR_RED);
    printf("Could not create thread.\n");
    printf("\t%s\n", strerror(errno));
    printf(ANSI_COLOR_RESET);
    exit(EXIT_FAILURE);
  } else if(VERBOSE) {
    printf(ANSI_COLOR_GREEN);
    printf("Keyboard thread started\n");
    printf(ANSI_COLOR_RESET);
  }

  pthread_join(keyboard_thread, NULL);

  pthread_cancel(keyboard_thread);

  return 0;
}

void sigalarm_handler(int signum) {
  refresh_remotes();
  alarm(refresh_interval);
}

void printHelp() {
  printf("\nUsage: console -c <ip> -c <ip> -r <secs>\n"
"\n"
"  Options:\n"
"\n"
"    -r <secs>      refresh interval\n"
"    -c <ip>        add computer\n"
"    -h             print usage info\n"
"    -v             verbose mode\n\n");
}

void sigint_handler(int signum) {
  cleanup();
  exit(0);
}

void cleanup() {
  remove_remote(0);
  if(command != NULL) {
    if(VERBOSE) {
      printf(ANSI_COLOR_CYAN);
      printf("Freeing command memory...\n");
      printf(ANSI_COLOR_RESET);
    }
    free(command);
    command = NULL;
  }
  if(refresh_sem != SEM_FAILED) {
    if(VERBOSE) {
      printf(ANSI_COLOR_CYAN);
      printf("Removing semaphore...\n");
      printf(ANSI_COLOR_RESET);
    }
    sem_close(refresh_sem);
    refresh_sem = SEM_FAILED;
    sem_unlink(DEFAULT_SEM_PATH);
  }
}

void* keyboard_do(void *argument) {
  size_t msg_length = sizeof(command);
  int bytes_read = 0;
  while((bytes_read = getline(&command, &msg_length, stdin)) != -1) {
    command[bytes_read-1] = '\0';
    if(strstr(command, LIST_MSG) == command) {
      // listing
      list_remotes();
    } else if(strstr(command, ADD_REMOTE_MSG) == command) {
      add_remote(command+strlen(ADD_REMOTE_MSG));
    } else if(strstr(command, REFRESH_MSG) == command) {
      refresh_remotes();
    } else if(strstr(command, STOP_MSG) == command) {
      refresh = false;
    } else if(strstr(command, START_MSG) == command) {
      refresh = true;
    } else if(strstr(command, REMOVE_MSG) == command) {
      remove_remote(atoi(command+strlen(REMOVE_MSG)));
    } else {
      // unknown command
      printf(ANSI_COLOR_YELLOW);
      printf("Unknown command!\n");
      printf(ANSI_COLOR_RESET);
    }
  }
  pthread_exit(NULL);
}

void read_from_server(struct remote *daemon) {
  int socket_number = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(socket_number < 0) {
    printf(ANSI_COLOR_RED);
    printf("Error creating socket.\n");
    printf(ANSI_COLOR_RESET);
    return;
  }
  
  struct sockaddr_in server_socket;
  socklen_t server_socket_length;
  memset(&server_socket, 0, sizeof server_socket);
  server_socket.sin_family = AF_INET;

  if(inet_pton(AF_INET, daemon->ip, &(server_socket.sin_addr)) <= 0) {
    printf(ANSI_COLOR_RED);
    printf("Error parsing remote IP address.\n");
    printf("\t%s\n", strerror(errno));
    printf(ANSI_COLOR_RESET);
    return;
  }

  server_socket.sin_port = htons(COMM_PORT);
  server_socket_length = sizeof(server_socket);

  if(connect(socket_number, (struct sockaddr *)&server_socket, server_socket_length) < 0) {
    printf(ANSI_COLOR_RED);
    printf("Error connecting.\n");
    printf("\t%s\n", strerror(errno));
    printf(ANSI_COLOR_RESET);
    return;
  }

  int received = 0;

  while((received = recv(socket_number, message_received, MAX_RMSG_LENGTH, 0)) > 0) {
    printf("%s", message_received);
    memset(message_received, 0, sizeof(message_received));
  }

  if(received < 0) {
    printf(ANSI_COLOR_RED);
    printf("Error receiving.\n");
    printf("\t%s\n", strerror(errno));
    printf(ANSI_COLOR_RESET);
    return;
  }

  close(socket_number);
}

void add_remote(char *ip) {
  if(VERBOSE) {
    printf(ANSI_COLOR_GREEN);
    printf("Adding remote with IP: %s.\n", ip);
    printf(ANSI_COLOR_RESET);
  }

  if(remotes == NULL) {
    // creating new list
    remotes = malloc(sizeof(struct remote));
    strncpy(remotes->ip, ip, MAX_IP_LEN);
    remotes->next = NULL;
  } else {
    // appending
    struct remote *head = remotes;
    while(head->next != NULL) {
      head = head->next;
    }
    head->next = malloc(sizeof(struct remote));
    strncpy(head->next->ip, ip, MAX_IP_LEN);
    head->next->next = NULL;
  }
}

void list_remotes() {
  printf("Registered remotes:\n");
  struct remote *head = remotes;
  int id = 1;
  while(head != NULL) {
    printf("  %d: %s\n", id, head->ip);
    head = head->next;
    id++;
  }
}

void remove_remote(int id) {
  if(id == 1) {
    struct remote *new_head = remotes->next;
    if(VERBOSE) {
      printf(ANSI_COLOR_CYAN);
      printf("Removing remote with IP %s.\n", remotes->ip);
      printf(ANSI_COLOR_RESET);
    }
    free(remotes);
    remotes = new_head;
  } else if (id > 1) {
    // remove one
    struct remote *prev = NULL;
    struct remote *head = remotes;
    int current_id = 1;
    while(current_id != id && head != NULL) {
      prev = head;
      head = head->next;
      current_id++;
    }
    if(head != NULL) {
      prev->next = head->next;
      if(VERBOSE) {
        printf(ANSI_COLOR_CYAN);
        printf("Removing remote with IP %s.\n", head->ip);
        printf(ANSI_COLOR_RESET);
      }
      free(head);
    }
  } else if(id == 0) {
    // remove all
    while(remotes != NULL) {
      remove_remote(1);
    }
  }
}

void refresh_remotes() {
  if(refresh == false) { return; }
  sem_wait(refresh_sem);
  struct remote *head = remotes;
  int i;
  printf("\e[1;1H\e[2J");
  for(i = 0; i<HLINE_LENGTH; i++) {
    printf("—");
  }
  printf("\n");
  while(head != NULL) {
    read_from_server(head);
    if(head->next != NULL) { printf("\n"); }
    head = head->next;
  }
  for(i = 0; i<HLINE_LENGTH; i++) {
    printf("—");
  }
  printf("\n");
  sem_post(refresh_sem);
}
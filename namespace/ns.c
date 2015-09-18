#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/capability.h>
#include <stdio.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>

#define STACK_SIZE (1024 * 1024)
static char container_stack[STACK_SIZE];

char* const container_args[] = {
  "/bin/bash",
  NULL
};

void format_print(const char *speeker, const char *content) {
  printf("%-10s [%5d] - %s\n", speeker, getpid(), content);
  return ;
}

int pipefd[2];

void set_map(char* file, int inside_id, int outside_id, int len) {
  FILE* mapfd = fopen(file, "w");
  if(NULL == mapfd) {
    perror("open file error");
    return ;
  }
  fprintf(mapfd, "%d %d %d", inside_id, outside_id, len);
  fclose(mapfd);
}

void set_uid_map(pid_t pid, int inside_id, int outside_id, int len) {
  char file[256];
  sprintf(file, "/proc/%d/uid_map", pid);
  set_map(file, inside_id, outside_id, len);
}

void set_gid_map(pid_t pid, int inside_id, int outside_id, int len) {
  char file[256];
  sprintf(file, "/proc/%d/gid_map", pid);
  set_map(file, inside_id, outside_id, len);
}

void format_string(char* str) {
  sprintf(str, "eUID = %ld, eGID = %ld, UID = %ld, GID = %ld"
      , (long)geteuid(), (long)getegid(), (long)getuid(), (long)getgid());
}

int container_main(void *arg) {
  format_print("Container", "inside the container!");
  char content[128];
  format_string(content);
  format_print("Container", content);
  
  char ch;
  close(pipefd[1]);
  read(pipefd[0], &ch, 1);

  format_print("Container", "setup hostname!");
  sethostname("container", 10);

  mount("proc", "/proc", "proc", 0, NULL);

  execv(container_args[0], container_args);
  format_print("Container", "Something's wrong!");
  return 1;
}

int main() {
  const int gid = getgid(), uid = getuid();

  char content[128];
  format_string(content);
  format_print("Parent", content);

  pipe(pipefd);

  format_print("Parent", "start a container!");
  int container_pid = clone(container_main, container_stack + STACK_SIZE
      , CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUSER | SIGCHLD, NULL);
  char content2[64];
  sprintf(content2, "Container [%5d]!", container_pid);
  format_print("Parent", content2);

  set_uid_map(container_pid, 0, uid, 1);
  set_gid_map(container_pid, 0, gid, 1);
  format_print("Parent", "user/group mapping done!");
  close(pipefd[1]);

  waitpid(container_pid, NULL, 0);
  format_print("Parent", "container stopped!");
  return 0;
}


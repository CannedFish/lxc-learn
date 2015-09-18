#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <stdio.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>

#define STACK_SIZE (1024 * 1024)
static char container_stack[STACK_SIZE];

char* const container_args[] = {
  "/bin/bash",
  "-l",
  NULL
};

void format_print(const char *speeker, const char *content) {
  printf("%-10s [%5d] - %s\n", speeker, getpid(), content);
  return ;
}

int container_main(void *arg) {
  format_print("Container", "inside the container!");
  sethostname("container", 10);

  if(mount("proc", "rootfs/proc", "proc", 0, NULL) != 0) {
    perror("proc");
  }
  if(mount("sysfs", "rootfs/sys", "sysfs", 0, NULL) != 0) {
    perror("sys");
  }
  if(mount("none", "rootfs/tmp", "tmpfs", 0, NULL) != 0) {
    perror("tmp");
  }
  if(mount("udev", "rootfs/dev", "devtmpfs", 0, NULL) != 0) {
    perror("dev");
  }
  if(mount("devpts", "rootfs/dev/pts", "devpts", 0, NULL) != 0) {
    perror("dev/pts");
  }
  if(mount("shm", "rootfs/dev/shm", "tmpfs", 0, NULL) != 0) {
    perror("dev");
  }
  if(mount("tmpfs", "rootfs/run", "tmpfs", 0, NULL) != 0) {
    perror("dev");
  }
  if(mount("conf/hosts", "rootfs/etc/hosts", "none", MS_BIND, NULL) != 0
      || mount("conf/hostname", "rootfs/etc/hostname", "none", MS_BIND, NULL) != 0
      || mount("conf/resolv.conf", "rootfs/etc/resolv.conf", "none", MS_BIND, NULL) != 0) {
    perror("conf");
  }
  // mount a local dir to the dir in container
  if(mount("/tmp/t1", "rootfs/mnt", "none", MS_BIND, NULL) != 0) {
    perror("mnt");
  }
  // set up the root dir
  if(chdir("./rootfs") != 0 || chroot("./") != 0) {
    perror("chdir/chroot");
  }

  execv(container_args[0], container_args);
  perror("exec");
  format_print("Container", "Something's wrong!");
  return 1;
}

int main() {
  format_print("Parent", "start a container!");
  int container_pid = clone(container_main, container_stack + STACK_SIZE
      , CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWPID | CLONE_NEWNS | SIGCHLD, NULL);
  waitpid(container_pid, NULL, 0);
  format_print("Parent", "container stopped!");
  return 0;
}


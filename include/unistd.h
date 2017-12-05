#ifndef _UNISTD_H
#define _UNISTD_H

#include <sys/defs.h>

//int open(const char *pathname, int flags);
//int close(int fd);
//ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int unlink(const char *pathname);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);

pid_t fork();
int execve(const char *file, char **const argv, char **const envp);
int execvp(char *file);
pid_t wait(int *status);
extern char **__environ;
#ifdef __USE_GNU
extern char **environ;
#endif
unsigned int sleep(unsigned int seconds);

pid_t getpid(void);
pid_t getppid(void);

// OPTIONAL: implement for ``on-disk r/w file system (+10 pts)''
off_t lseek(int fd, off_t offset, int whence);
//int mkdir(const char *pathname, mode_t mode);

// OPTIONAL: implement for ``signals and pipes (+10 pts)''
int pipe(int *pipefd);
void yield();
int dup2(int oldfd, int newfd);
#endif

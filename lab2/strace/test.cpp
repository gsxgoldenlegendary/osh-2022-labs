#include <sys/ptrace.h>
#include <sys/user.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
 
 
int main(int argc, char *argv[]){
    pid_t pid;
    int status;
    //struct user_regs_struct regs;
    int orig_rax;
    pid = fork();
    if((pid)==-1)
    exit(0);
    if (pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execvp(argv[1], argv+1);
        exit(1);
    } else {
        wait(&status); // 接收被子进程发送过来的 SIGCHLD 信号
        //ptrace(PTRACE_SYSCALL, pid, 0, 0);

        struct user_regs_struct regs;

        while (1) {
            // 1. 发送 PTRACE_SYSCALL 命令给被跟踪进程 (调用系统调用前，可以获取系统调用的参数)
            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
            wait(&status); // 接收被子进程发送过来的 SIGCHLD 信号
            if (WIFEXITED(status)) { // 如果子进程退出了, 那么终止跟踪
                break;
            }
            //waitpid(pid, 0, 0);
            ptrace(PTRACE_GETREGS, pid, 0, &regs);
            long syscall = regs.orig_rax;
            fprintf(stderr, "%ld(%ld, %ld, %ld, %ld, %ld, %ld)\n", syscall,
            (long)regs.rdi, (long)regs.rsi, (long)regs.rdx,
            (long)regs.r10, (long)regs.r8,  (long)regs.r9
            );
            ptrace(PTRACE_SYSCALL, pid, 0, 0);
 
            wait(&status); // 接收被子进程发送过来的 SIGCHLD 信号
            if (WIFEXITED(status)) { // 如果子进程退出了, 那么终止跟踪
                break;
            }
            //waitpid(pid, 0, 0);
        }
    }
    return 0;
}
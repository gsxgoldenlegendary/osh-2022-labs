#include <sys/ptrace.h>
#include <sys/user.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <sys/wait.h>

int main(__attribute__((unused)) int argc, char *argv[]) {
    pid_t pid;
    int status;
    //struct user_regs_struct regs;
    pid = fork();
    if (pid == -1)
        exit(0);
    else if (pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execvp(argv[1], argv + 1);
        exit(1);
    } else {
        wait(&status);
        struct user_regs_struct regs{};
        while (true) {
            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
            wait(&status);
            if (WIFEXITED(status)) {
                break;
            }
            ptrace(PTRACE_GETREGS, pid, 0, &regs);
            long syscall = long(regs.orig_rax);
            fprintf(stderr, "%ld(%ld, %ld, %ld, %ld, %ld, %ld)\n", syscall,
                    (long) regs.rdi, (long) regs.rsi, (long) regs.rdx,
                    (long) regs.r10, (long) regs.r8, (long) regs.r9
            );
            ptrace(PTRACE_SYSCALL, pid, 0, 0);
            wait(&status);
            if (WIFEXITED(status)) {
                break;
            }
        }
    }
    return 0;
}


#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
char a[5];
char b[20];
int main(){
	long ret;
	printf("system call begin\n");
	ret=syscall(548,a,5);
	if(ret==-1)
		printf("System call succeeded!n%s\n",a);
	else
		printf("%ld,ERROR\n",ret);
	ret=syscall(548,b,20);
	if(ret==0)
		printf("System call succeeded!n%s\n",b);
	else
		printf("%ld,ERROR\n",ret);
	while(1);
	return 0;
}
/*#define _GNU_SOURCE
       #include <unistd.h>
       #include <sys/syscall.h>
       #include <sys/types.h>
       #include <signal.h>

       int
       main(int argc, char *argv[])
       {
           pid_t tid;

           tid = syscall(SYS_gettid);
           syscall(SYS_tgkill, getpid(), tid, SIGHUP);
       }*/

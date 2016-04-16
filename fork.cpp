#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <cstdio>

using namespace std;
const int maxn = 100;

int main()
{

    int fd[2];
    if(pipe(fd) == -1)
        cerr << "Can't create the pipe" << endl;
    pid_t pid = fork();
    char* args[10] = {"grep", "g++", "/home/magnolias/Github/Mshell/Makefile", NULL};
    if(pid == 0)
    {
        execvp(args[0], args);
    }
    else
    {
        int pid_status;
        //waitpid(pid, &pid_status, 0);
        cout << "这是父进程" << endl;
    }
}

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <cstdio>
#include <sstream>
#include <cstring>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

const int maxn = 100;
struct Process;
struct Job;
int handleInput(Process commands[]);
void set_term_color();
void reset_term_color();

enum FLAGS{APPEND, TRUNC, NONE};
struct Process
{
    FLAGS flag = NONE;
    pid_t pid;
    Process* next = NULL;
    char fin[maxn] = {'\0'};
    char fout[maxn] = {'\0'};
    char *args[maxn] = {NULL};
    int index = 0;
    void addArg(char* arg)
    {
        args[index++] = arg;
    }
    ~Process()
    {
        for (int i = 0; args[i] != NULL; i++)
        {
            delete args[i];
        }
    }
};
struct Job
{
    Process* firstProcess = NULL;
    Job* next = NULL;
    pid_t pgid;
};
void initShell()
{
}
int main()
{
    set_term_color();
    chdir(getenv("HOME"));
    char pwd[1024];
    getcwd(pwd, 1024);
    while(1)
    {
        Process commands[100];
        int num = handleInput(commands);
        /*
        for (int i = 0; i < num; i++)
        {
            cout << commands[i].args[0] << endl;
            cout << (strlen(commands[i].fin) == 0 ? "输入重定向为空" : commands[i].fin) << endl;
            switch(commands[i].flag)
            {
                case NONE : cout << "none" << endl;break;
                case APPEND : cout << "append" << endl; break;
                case TRUNC : cout << "trunc" << endl; break;
            }
            cout << (strlen(commands[i].fout) == 0 ? "输出重定向为空" : commands[i].fout) << endl;
            cout << strlen(commands[i].args[0]) << endl;
            cout << strlen(commands[i].args[1]) << endl;
            for (int j = 0; commands[i].args[j] != NULL && strlen(commands[i].args[j]) != 0; ++j)
                cout << commands[i].args[j] << "\t";
            cout << endl;
        }
        */

        if(num == -1)
        {
            cerr << "解析命令出错" << endl;
            return 0;
        }
        else if(num == -2)
        {
            continue;
        }
        else if(num == 0)
            continue;
        else if(strcmp(commands[0].args[0], "exit") == 0)
        {
            cout << "Bye~" << endl;
            reset_term_color();
        	return 0;
        }
        else if(strcmp(commands[0].args[0], "cd") == 0)
        {
            chdir(commands[0].args[1]);
        }
        else
        {
            int infile = STDIN_FILENO, outfile = STDOUT_FILENO;
            int pipefd[2];
            for (int i = 0; i < num; i++)
            {
                if(i != num - 1)
                {
                    if(pipe(pipefd) == -1)
                    {
                        cerr << "打开管道失败" << endl;
                        return 0;
                    }
                    outfile = pipefd[1];
                }
                else
                    outfile = STDOUT_FILENO;
                if(strlen(commands[i].fin) != 0)
                {
                    infile = open(commands[i].fin, O_RDONLY);
                    if(infile < 0)
                    {
                        cerr << "打开文件失败" << commands[i].fin << endl;
                        return 0;
                    }
                }
                if(strlen(commands[i].fout) != 0)
                {
                    if((outfile = open(commands[i].fout, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
                    {
                        cerr << "打开文件失败" << commands[i].fout << endl;
                        return 0;
                    }
                }
                pid_t pid = fork();
                if(pid == 0)
                {
                    if(infile != STDIN_FILENO)
                    {
                        dup2(infile, 0);
                        close(infile);
                    }
                    if(outfile != STDOUT_FILENO)
                    {
                        dup2(outfile, 1);
                        close(outfile);
                    }
                    if(strcmp(commands[i].args[0], "wc") == 0)
                    {
                        char *name = new char[4];
                        strcpy(name, "Mwc");
                        delete commands[i].args[0];
                        commands[i].args[0] = name;
                        execv("/home/magnolias/Github/Mshell/bin/Mwc", commands[i].args);
                    }
                    else if(strcmp(commands[i].args[0], "cp") == 0)
                    {
                        char *name = new char[4];
                        strcpy(name, "Mcp");
                        delete commands[i].args[0];
                        commands[i].args[0] = name;
                        execv("/home/magnolias/Github/Mshell/bin/Mcp", commands[i].args);
                    }
                    else
                    {
                        execvp(commands[i].args[0], commands[i].args);
                    }
                }
                else
                {
                    int status;
                    if(i == num - 1)
                        if(waitpid(pid, &status, 0) == -1)
                            cerr << "error when waiting process terminated" << endl;
                }
            }
            if(infile != STDIN_FILENO)
                close(infile);
            if(outfile != STDOUT_FILENO)
                close(outfile);
            infile = pipefd[0];
        }
    }
}

int handleInput(Process *commands)
{
	char shell_prompt[100];
    // Configure readline to auto-complete paths when the tab key is hit.
    rl_bind_key('\t', rl_complete);
    snprintf(shell_prompt, sizeof(shell_prompt), "%s:%s $ ", getenv("USER"), getcwd(NULL, 1024));
    char* cmd = NULL;
    cmd = readline(shell_prompt);
    bool isValid = false;
    if(cmd == NULL)
    {
        cerr << "没有得到命令" << endl;
        return -2;
    }
    for (int i = 0; cmd[i] != '\0'; i++)
        if(!isspace(cmd[i]))
            isValid = true;
    if(!isValid)
        return -2;
    int index = 0;
    int j = 0, i = 0;
    bool isEmpty = true;
    while(cmd[i] != '\0')
    {
        isEmpty = false;
        if(isspace(cmd[i]))
        {
            i++;
            continue;
        }
        else if(cmd[i] == '<')
        {
            if(cmd[i + 1] == '\0')
            {
                cerr << "输入重定向缺少文件";
                return -1;
            }
            if(cmd[i + 1] == '<')
            {
                bool hasGotFile = false;
                int k = 0;
                for(j = i + 2; cmd[j] != '\0' && cmd[j] != '<' && cmd[j] != '>' && cmd[j] != '|'; ++j)
                {
                    if(isspace(cmd[j]) && hasGotFile)
                        break;
                    else if(isspace(cmd[j]) && !hasGotFile)
                        continue;
                    commands[index].fin[k++] = cmd[j];
                    hasGotFile = true;
                }
                commands[index].fin[k] = '\0';
                i = j;
                continue;
            }
            else
            {
                bool hasGotFile = false;
                int k = 0;
                for(j = i + 1; cmd[j] != '\0' && cmd[j] != '<' && cmd[j] != '>' && cmd[j] != '|'; ++j)
                {
                    if(isspace(cmd[j]) && hasGotFile)
                        break;
                    else if(isspace(cmd[j]) && !hasGotFile)
                        continue;
                    commands[index].fin[k++] = cmd[j];
                    hasGotFile = true;
                }
                commands[index].fin[k] = '\0';
                i = j;
                continue;
            }
        }
        else if(cmd[i] == '>')
        {
            if(cmd[i + 1] == '\0')
            {
                cerr << "输出重定向缺少文件";
                return -1;
            }
            if(cmd[i + 1] == '>')
            {
                bool hasGotFile = false;
                int k = 0;
                for(j = i + 2; cmd[j] != '\0' && cmd[j] != '<' && cmd[j] != '>' && cmd[j] != '|'; ++j)
                {
                    if(isspace(cmd[j]) && hasGotFile)
                        break;
                    else if(isspace(cmd[j]) && !hasGotFile)
                        continue;
                    commands[index].fout[k++] = cmd[j];
                    hasGotFile = true;
                }
                commands[index].fout[k] = '\0';
                commands[index].flag = APPEND;
                i = j;
                continue;
            }
            else
            {
                bool hasGotFile = false;
                int k = 0;
                for(j = i + 1; cmd[j] != '\0' && cmd[j] != '<' && cmd[j] != '>' && cmd[j] != '|'; ++j)
                {
                    if(isspace(cmd[j]) && hasGotFile)
                        break;
                    else if(isspace(cmd[j]) && !hasGotFile)
                        continue;
                    commands[index].fout[k++] = cmd[j];
                    hasGotFile = true;
                }
                commands[index].fout[k] = '\0';
                commands[index].flag = TRUNC;
                i = j;
                continue;
            }
        }
        else if(cmd[i] == '-') 
        {
            char* arg = new char[100];
            for(j = i; cmd[j] != '\0' && cmd[j] != '<' && cmd[j] != '>' && cmd[j] != '|' && !isspace(cmd[j]); ++j)
                arg[j - i] = cmd[j];
            arg[j - i] = '\0';
            commands[index].addArg(arg);
            i = j;
            continue;
        }
        else if(cmd[i] == '|')
        {
            index++;
            i++;
            continue;
        }
        else
        {
            char* arg = new char[100];
            for(j = i; cmd[j] != '\0' && cmd[j] != '<' && cmd[j] != '>' && cmd[j] != '|' && !isspace(cmd[j]); ++j)
                arg[j - i] = cmd[j];
            arg[j - i] = '\0';
            commands[index].addArg(arg);
            i = j;
            continue;
        }
    }
    // Add input to history.
    add_history(cmd);
    free(cmd);
    if(isEmpty)
        return 0;
    return index + 1;
}

int launchProcess(Process subprocess)
{
}


void set_term_color()
{  
    fprintf(stdout, "\033[33m");  
    fflush(stdout);  
}

void reset_term_color()
{  
    fprintf(stdout, "\033[0m");  
    fflush(stdout);  
}

int pipe()
{
    return 0;
}

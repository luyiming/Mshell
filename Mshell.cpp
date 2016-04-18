#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <vector>
#include <cstring>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>
#include <cerrno>
#include <signal.h>
#include <termios.h>
#include <ncurses.h>

using namespace std;

const int maxn = 100;
struct Process
{
    Process *next = NULL;       /* next process in pipeline */
    Process *prev = NULL;
    vector<string> args;
    pid_t pid = 0;                  /* process ID */
    char completed = 0;             /* true if process has completed */
    char stopped = 0;               /* true if process has stopped */
    int status = 0;                 /* reported status value */
    string infile;
    string outfile;
    string errfile;
    int in_fd = STDIN_FILENO;
    int out_fd = STDOUT_FILENO;
    int err_fd = STDERR_FILENO;
    bool output_append = false;
    int exec();
};

struct Job
{
    Job *next = NULL;           /* next active job */
    Job *prev = NULL;
    string command;
    Process *process_head = NULL;     /* list of processes in this job */
    pid_t pgid = 0;                 /* process group ID */
    char notified = 0;              /* true if user told about stopped job */
    struct termios tmodes;      /* saved terminal modes */
    ~Job()
    {
        if(process_head != NULL)
        {
            Process* p = process_head->next;
            while(p != NULL)
            {
                delete p->prev;
                process_head = p;
                p = p->next;
            }
            delete process_head;
        }
    }
};


Job* handleInput();
void set_term_color();
void reset_term_color();
void man(char *args[]);

Job *findJob(pid_t pgid);
bool job_is_stopped (Job *j);
bool job_is_completed (Job *j);
void launch_process (Process *p, pid_t pgid, int foreground);
void launch_job (Job *j, int foreground);
void put_job_in_foreground (Job *j, int cont);
int mark_process_status (pid_t pid, int status);
void put_job_in_background (Job *j, int cont);
void update_status (void);
void wait_for_job (Job *j);
void do_job_notification (void);
void mark_job_as_running (Job *j);
void continue_job (Job *j, int foreground);
void format_job_info (Job *j, const char *status);
bool built_in_func(Job *j);


pid_t shell_pgid;
struct termios shell_tmodes;
int shell_fd;
int shell_is_interactive;

void init_shell()
{
    shell_fd = STDIN_FILENO;
    shell_is_interactive = isatty(shell_fd);

    if (shell_is_interactive)
    {
        /* Loop until we are in the foreground.  */
        while(tcgetpgrp(shell_fd) != (shell_pgid = getpgrp()))
            kill(-shell_pgid, SIGTTIN);

        signal (SIGINT, SIG_IGN);
        signal (SIGQUIT, SIG_IGN);
        signal (SIGTSTP, SIG_IGN);
        signal (SIGTTIN, SIG_IGN);
        signal (SIGTTOU, SIG_IGN);
        //signal (SIGCHLD, SIG_IGN); //------------attention

        /* Put ourselves in our own process group.  */
        shell_pgid = getpid();
        if(setpgid(shell_pgid, shell_pgid) < 0)
        {
            fprintf(stderr, "Couldn't put the shell in its own process group");
            exit(1);
        }
        tcsetpgrp (shell_fd, shell_pgid);
        tcgetattr (shell_fd, &shell_tmodes);
    }

    set_term_color();
    chdir(getenv("HOME"));
    printf("+---------------------------+\n");
    printf("|                           |\n");
    printf("|        Mshell v1.0        |\n");
    printf("|                           |\n");
    printf("|          Welcome!         |\n");
    printf("|                           |\n");
    printf("+---------------------------+\n");
}

int Process::exec()
{
    char* argv[100];
    int index = 0;
    for (vector<string>::size_type i = 0; i < args.size(); i++)
    {
        char* t = new char[args[i].size() + 1];
        strcpy(t, args[i].c_str());
        argv[index++] = t;
    }
    argv[index] = NULL;
    if(args[0] == "cp" || args[0] == "wc" || args[0] == "cmp" || args[0] == "cat")
    {
        delete argv[0];
        args[0].insert(0, "M");
        char* t = new char[args[0].size() + 1];
        strcpy(t, args[0].c_str());
        argv[0] = t;
        execv((string("/home/magnolias/Mshell/bin/") + args[0]).c_str(), argv);
        perror("launch process error : ");
        exit(1);
    }
    else if(args[0] == "man")
    {
        man(argv);
        exit(0);
    }
    else
    {
        execvp(argv[0], argv);
        perror("launch process error : ");
        exit(1);
    }
}



Job *job_head = NULL;
Job *job_tail = NULL;

int main()
{
    init_shell();
    while(1)
    {
        set_term_color();
        Job* j = handleInput();
        if(j == NULL)
        {
            cerr << "输入错误" << endl;
            continue;
        }
        if(built_in_func(j))
            continue;
        if(job_head == NULL)
            job_head = j;
        else
        {
            job_tail->next = j;
            j->prev = job_tail;
        }
        job_tail = j;

        launch_job(job_tail, 1);
        do_job_notification();
    }
}
bool built_in_func(Job* j)
{
    if(j->command == "exit")
    {
        printf("+---------------------------+\n");
        printf("|                           |\n");
        printf("|        Mshell v1.0        |\n");
        printf("|                           |\n");
        printf("|           Bye~~           |\n");
        printf("|                           |\n");
        printf("+---------------------------+\n");
        reset_term_color();
        exit(0);
    }
    else if(j->process_head->args[0] == "cd")
    {
        if(j->process_head->args.size() < 2)
            cerr << "cd 缺少路径参数" << endl;
        else if(j->process_head->args.size() > 2)
            return true;
        else if(chdir(j->process_head->args[1].c_str()) == -1)
            perror("cd error : ");
        return true;
    }
    return false;
}

Job* handleInput()
{
	char shell_prompt[100];
    // Configure readline to auto-complete paths when the tab key is hit.
    rl_bind_key('\t', rl_complete);
    snprintf(shell_prompt, sizeof(shell_prompt), "%s:%s $ ", getenv("USER"), getcwd(NULL, 1024));
    char* cmd = NULL;
    cmd = readline(shell_prompt);
    if(cmd == NULL)
    {
        cerr << "没有得到命令" << endl;
        return NULL;
    }
    bool isEmpty = true;
    for(int i = 0; cmd[i] != '\0'; i++)
        if(!isspace(cmd[i]))
        {
            isEmpty = false;
            break;
        }
    if(isEmpty)
        return NULL;
    Job *job = new Job;
    job->process_head = new Process;
    job->command = string(cmd);
    Process *process_tail = job->process_head;
    
    
    int j = 0, i = 0;
    while(cmd[i] != '\0')
    {
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
                return NULL;
            }
            if(cmd[i + 1] == '<')
            {
                bool hasGotFile = false;
                process_tail->infile.clear();
                for(j = i + 2; cmd[j] != '\0' && cmd[j] != '<' && cmd[j] != '>' && cmd[j] != '|'; ++j)
                {
                    if(isspace(cmd[j]) && hasGotFile)
                        break;
                    else if(isspace(cmd[j]) && !hasGotFile)
                        continue;
                    process_tail->infile += cmd[j];
                    hasGotFile = true;
                }
                i = j;
                continue;
            }
            else
            {
                bool hasGotFile = false;
                process_tail->infile.clear();
                for(j = i + 1; cmd[j] != '\0' && cmd[j] != '<' && cmd[j] != '>' && cmd[j] != '|'; ++j)
                {
                    if(isspace(cmd[j]) && hasGotFile)
                        break;
                    else if(isspace(cmd[j]) && !hasGotFile)
                        continue;
                    process_tail->infile += cmd[j];
                    hasGotFile = true;
                }
                i = j;
                continue;
            }
        }
        else if(cmd[i] == '>')
        {
            if(cmd[i + 1] == '\0')
            {
                cerr << "输出重定向缺少文件";
                return NULL;
            }
            if(cmd[i + 1] == '>')
            {
                bool hasGotFile = false;
                process_tail->outfile.clear();
                for(j = i + 2; cmd[j] != '\0' && cmd[j] != '<' && cmd[j] != '>' && cmd[j] != '|'; ++j)
                {
                    if(isspace(cmd[j]) && hasGotFile)
                        break;
                    else if(isspace(cmd[j]) && !hasGotFile)
                        continue;
                    process_tail->outfile += cmd[j];
                    hasGotFile = true;
                }
                process_tail->output_append = true;
                i = j;
                continue;
            }
            else
            {
                bool hasGotFile = false;
                process_tail->outfile.clear();
                for(j = i + 1; cmd[j] != '\0' && cmd[j] != '<' && cmd[j] != '>' && cmd[j] != '|'; ++j)
                {
                    if(isspace(cmd[j]) && hasGotFile)
                        break;
                    else if(isspace(cmd[j]) && !hasGotFile)
                        continue;
                    process_tail->outfile += cmd[j];
                    hasGotFile = true;
                }
                process_tail->output_append = false;
                i = j;
                continue;
            }
        }
        else if(cmd[i] == '-') 
        {
            string arg;
            for(j = i; cmd[j] != '\0' && cmd[j] != '<' && cmd[j] != '>' && cmd[j] != '|' && !isspace(cmd[j]); ++j)
                arg += cmd[j];
            process_tail->args.push_back(arg);
            i = j;
            continue;
        }
        else if(cmd[i] == '|')
        {
            process_tail->next = new Process;
            process_tail->next->prev = process_tail;
            process_tail = process_tail->next;
            i++;
            continue;
        }
        else
        {
            string arg;
            for(j = i; cmd[j] != '\0' && cmd[j] != '<' && cmd[j] != '>' && cmd[j] != '|' && !isspace(cmd[j]); ++j)
                arg += cmd[j];
            process_tail->args.push_back(arg);
            i = j;
            continue;
        }
    }
    // Add input to history.
    add_history(cmd);
    free(cmd);
    return job;
}

void man(char *args[])
{
    string manPath = "/home/magnolias/Github/Mshell/man/";
    manPath += args[1];
    manPath += ".man";
    vector<string> text;
    ifstream fin(manPath.c_str());
    if(!fin)
    {
        cerr << "open file error" << endl;
        pid_t pid = getpid();
        kill(pid, SIGINT);
        return;
    }
    string temp;
    while(getline(fin, temp))
        text.push_back(temp);
    fin.close();
    int ch, row, col;
    int y = 0, x = 0;
    initscr();				/* Start curses mode */
    cbreak();
    clear();
    noecho();
    keypad(stdscr, true);
    start_color();
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    getmaxyx(stdscr, row, col);		/* find the boundaries of the screeen */
    int startPos = 0;
    while(1)	/* read the file till we reach the end */
    {
        clear();
        move(0, 0);			/* start at the beginning of the screen */
        attron(COLOR_PAIR(1));
        for(int i = 0; i < row - 1; i++)
            printw("%s\n", text[startPos + i].c_str());
        attroff(COLOR_PAIR(1));
        attron(A_STANDOUT | COLOR_PAIR(3));			/* cut bold on */
        printw("Manual page %s line %d (press q to quit)", args[1], startPos + 1);
        attroff(A_STANDOUT | COLOR_PAIR(3));			/* cut bold on */
        refresh();
        ch = getch();
        switch(ch)
        {
            case KEY_UP:
                startPos = (startPos - 1 >= 0 ? startPos - 1 : 0); break;
            case KEY_DOWN:
                startPos = (startPos + 1 < text.size() - row ? startPos + 1 : text.size() - row - 1); break;
            case 'q':
                endwin();
                pid_t pid = getpid();
                kill(pid, SIGINT);
                return;
        }
    }
    endwin();                       	/* End curses mode */
    return;
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


Job *findJob(pid_t pgid)
{
    for(Job *j = job_head; j != NULL; j = j->next)
        if (j->pgid == pgid)
            return j;
    return NULL;
}

bool job_is_stopped(Job *j)
{
    for(Process *p = j->process_head; p != NULL; p = p->next)
        if (!p->stopped)
            return false;
    return true;
}

bool job_is_completed(Job *j)
{
    for(Process *p = j->process_head; p != NULL; p = p->next)
        if (!p->completed)
            return false;
    return true;
}

void launch_process(Process *p, pid_t pgid, int foreground)
{
    pid_t pid;

    if (shell_is_interactive)
    {
        pid = getpid();
        if(pgid == 0) 
            pgid = pid;
        setpgid(pid, pgid);
        if(foreground)
            tcsetpgrp(shell_fd, pgid);

        signal (SIGINT, SIG_DFL);
        signal (SIGQUIT, SIG_DFL);
        signal (SIGTSTP, SIG_DFL);
        signal (SIGTTIN, SIG_DFL);
        signal (SIGTTOU, SIG_DFL);
        signal (SIGCHLD, SIG_DFL);
    }

    if(p->in_fd != STDIN_FILENO)
    {
        dup2(p->in_fd, STDIN_FILENO);
        close(p->in_fd);
    }
    if(p->out_fd != STDOUT_FILENO)
    {
        dup2(p->out_fd, STDOUT_FILENO);
        close(p->out_fd);
    }
    if(p->err_fd != STDERR_FILENO)
    {
        dup2(p->err_fd, STDERR_FILENO);
        close(p->err_fd);
    }

    p->exec();
}

void launch_job(Job *j, int foreground)
{
    Process *p;
    pid_t pid;
    int pipefd[2], infile, outfile;

    infile = STDIN_FILENO;
    for (p = j->process_head; p != NULL; p = p->next)
    {
        if (p->next != NULL)
        {
            if (pipe(pipefd) < 0)
            {
                perror("make pipe error");
                exit (1);
            }
            outfile = pipefd[1];
            p->out_fd = pipefd[1];
        }
        else
            outfile = STDOUT_FILENO;
        p->in_fd = infile;
        if(!p->infile.empty())
        {
            if((p->in_fd = open(p->infile.c_str(), O_RDONLY)) == -1)
            {
                perror("open input redirection file error : ");
                return;
            }
        }
        if(!p->outfile.empty())
        {
            if((p->out_fd = open(p->outfile.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
            {
                perror("open output redirection file error : ");
                return;
            }
        }
        pid = fork();
        if (pid == 0)
            launch_process(p, j->pgid, foreground);
        else if(pid < 0)
        {
            cerr << "fork child process error" << endl;
            exit (1);
        }
        else
        {
            p->pid = pid;
            if (shell_is_interactive)
            {
                if(j->pgid == 0)
                    j->pgid = pid;
                setpgid(pid, j->pgid);
            }
        }
        if(infile != STDIN_FILENO)
            close(infile);
        if(outfile != STDOUT_FILENO)
            close(outfile);
        infile = pipefd[0];
    }

    format_job_info(j, "launched");

    if(!shell_is_interactive)
        wait_for_job(j);
    else if(foreground)
        put_job_in_foreground(j, 0);
    else
        put_job_in_background(j, 0);
}

/* Put job j in the foreground.  If cont is nonzero,
   restore the saved terminal modes and send the process group a
   SIGCONT signal to wake it up before we block.  */

void put_job_in_foreground (Job *j, int cont)
{
    /* Put the job into the foreground.  */
    tcsetpgrp(shell_fd, j->pgid);
    /* Send the job a continue signal, if necessary.  */
    if(cont)
    {
        tcsetattr(shell_fd, TCSADRAIN, &j->tmodes);
        if (kill(- j->pgid, SIGCONT) < 0)
            perror ("kill (SIGCONT)");
    }
    wait_for_job(j);

    /* Put the shell back in the foreground.  */
    tcsetpgrp (shell_fd, shell_pgid);

    /* Restore the shell’s terminal modes.  */
    tcgetattr (shell_fd, &j->tmodes);
    tcsetattr (shell_fd, TCSADRAIN, &shell_tmodes);
}

/* Put a job in the background.  If the cont argument is true, send
   the process group a SIGCONT signal to wake it up.  */

void put_job_in_background (Job *j, int cont)
{
    if (cont)
        if (kill(- j->pgid, SIGCONT) < 0)
            perror ("kill (SIGCONT)");
}

int mark_process_status (pid_t pid, int status)
{
    Job *j;
    Process *p;
    if(pid > 0)
    {
        /* Update the record for the process.  */
        for (j = job_head; j != NULL; j = j->next)
            for (p = j->process_head; p != NULL; p = p->next)
                if (p->pid == pid)
                {
                    p->status = status;
                    if (WIFSTOPPED(status))
                    {
                        p->stopped = 1;
                        fprintf (stderr, "%d: Stoped by signal %d.\n", (int)pid, WSTOPSIG(p->status)); 
                    }
                    else
                    {
                        p->completed = 1;
                        if (WIFSIGNALED(status))
                            fprintf (stderr, "%d: Terminated by signal %d.\n", (int)pid, WTERMSIG(p->status));
                    }
                    return 0;
                }
        fprintf (stderr, "No child process %d.\n", pid);
        return -1;
    }
    else if (pid == 0 || errno == ECHILD)
        /* No processes ready to report.  */
        return -1;
    else
    {
        /* Other weird errors.  */
        perror ("waitpid");
        return -1;
    }
}

/* Check for processes that have status information available,
without blocking.  */
void update_status (void)
{
    int status;
    pid_t pid;
    do
        pid = waitpid (WAIT_ANY, &status, WUNTRACED | WNOHANG);
    while (!mark_process_status(pid, status));
}

/* Check for processes that have status information available,
blocking until all processes in the given job have reported.  */
void wait_for_job (Job *j)
{
    int status;
    pid_t pid;

    do
        pid = waitpid (WAIT_ANY, &status, WUNTRACED);
    while (!mark_process_status (pid, status)
            && !job_is_stopped (j)
            && !job_is_completed (j));
}

/* Format information about job status for the user to look at.  */
void format_job_info (Job *j, const char *status)
{
    fprintf(stderr, "%d (%s): %s\n", j->pgid, status, j->command.c_str());
}

/* Notify the user about stopped or terminated jobs.
   Delete terminated jobs from the active job list.  */
void do_job_notification (void)
{
    Job *j = job_head;

    update_status();

    while(j != NULL)
    {
        if (job_is_completed(j))
        {
            format_job_info(j, "completed");
            if (j == job_head)
            {
                job_head = job_head->next;
                if(job_head != NULL)
                    job_head->prev = NULL;
                else
                    job_tail = NULL;
                delete j;
                j = job_head;
            }
            else if(j->next != NULL)
            {
                j->next->prev = j->prev;
                j->prev->next = j->next;
                Job* t = j;
                j = j->next;
                delete t;
            }
            else
            {
                j->prev->next = NULL;
                job_tail = j->prev;
                delete j;
                j = NULL;
            }
            continue;
        }
        else if(job_is_stopped(j) && !j->notified)
        {
            format_job_info (j, "stopped");
            j->notified = 1;
        }
        j = j->next;
    }
}

/* Mark a stopped job J as being running again.  */
void mark_job_as_running (Job *j)
{
    for (Process *p = j->process_head; p != NULL; p = p->next)
        p->stopped = 0;
    j->notified = 0;
}

void continue_job (Job *j, int foreground)
{
    mark_job_as_running (j);
    if (foreground)
        put_job_in_foreground (j, 1);
    else
        put_job_in_background (j, 1);
}


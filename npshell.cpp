using namespace std;
#include <iostream>
#include <cstring>
#include <deque>
#include <vector>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#ifndef DEBUG
#define DEBUG_BLOCK(statement)
#else
#define DEBUG_BLOCK(statement) do { statement } while (0)
#endif

#define MAX_ARGUMENTS 10

deque<char*> cmd;
class my_proc {
public:
    my_proc(char* cname): cname(cname), next(nullptr), arg_count(1), arg_list{cname, NULL}, line_count(-1), completed(false), pid(-1) {}
    my_proc* next;
    deque<my_proc*> prev;
    char* cname;
    int arg_count;
    char* arg_list[10];
    int line_count;
    bool completed;
    int pid;


};
deque<my_proc*> proc;
deque<my_proc*>::iterator proc_ptr = proc.end();

void create_pipe(int* pipefd) {

    if (pipe(pipefd) < 0) {
        cerr << "pipe error" << endl;
    }

    return;
}

bool check_unknown(const char cmd_name[]) {

    DEBUG_BLOCK (
                 cout << "check unknown: " << cmd_name << endl;
                 );
    if (access(cmd_name, F_OK) != -1) {
            DEBUG_BLOCK (
                 cout << "known: " << cmd_name << endl;
                 );
        return false;
    }
    char* env = (char*)malloc(sizeof(char)*100);
    //getenv("PATH");
    strcpy(env, getenv("PATH"));

    char* p = NULL;
    do {
        p = strchr(env, ':');
        if (p != NULL) {
            p[0] = '\0';
        }
        char path[256];
        strcpy(path, env);
        if (path[strlen(path)-1] != '/') {
            strcat(path, "/");
        }
        strcat(path, cmd_name);
        DEBUG_BLOCK (
                     cout << "path:" << path << endl;
                     );

        if (access(path, F_OK) != -1) {
                DEBUG_BLOCK (
                 cout << "known: " << cmd_name << endl;
                 );
            return false;
        }
        env = p+1;
    } while (p != NULL);

    cerr << "Unknown command: [" << cmd_name << "]." << endl;
    return true;

}

void read_pipe(int readfd) {

    close(0);
    dup(readfd);
    return;
}
void write_pipe(int writefd) {

    close(1);
    dup(writefd);
    return;
}
void close_pipe(int fd1, int fd2) {

    close(fd1);
    close(fd2);
    return;
}


void do_pipe(my_proc* cur, my_proc* nxt) {
    //cout << "do pipe" << endl;
    int pipefd[2];
    int childpid1, childpid2;
    int wstatus;
    //proc_ptr+=2;
    DEBUG_BLOCK (
                 cout << cur->cname << nxt->cname << endl;
                 );

    // check unknown command
    /*if (check_unknown(cur->cname)) {
        proc_ptr--;
        return;
    }*/
    if (check_unknown(nxt->cname)) {
        nxt->completed = true;
        return;
    }
    create_pipe(&pipefd[0]);

    nxt->pid = fork();
    if (nxt->pid == -1) {
        cerr << "can't fork" << endl;
    }
    else if (nxt->pid == 0) {
        //child process
        read_pipe(pipefd[0]);
        close_pipe(pipefd[0], pipefd[1]);
        execvp(nxt->cname, nxt->arg_list);

        exit(0);
    }
    else {
        //parent process
        for (int i = 0; i < nxt->prev.size(); i++) {

            my_proc* p = nxt->prev[i];

            // check unknown command
            if (check_unknown(p->cname)) {
                p->completed = true;
                continue;
            }
            if (i != 0) {
                waitpid(nxt->prev[i-1]->pid, &wstatus, 0);
            }
            p->pid = fork();
            if (p->pid == -1) {
                cerr << "can't fork" << endl;
            }
            else if (p->pid == 0) {
                // child process

                write_pipe(pipefd[1]);
                close_pipe(pipefd[0], pipefd[1]);
                execvp(p->cname, p->arg_list);

                exit(0);
            }
            else {
                //parent process
                close_pipe(pipefd[0], pipefd[1]);
                /*if (waitpid(childpid1, &wstatus, 0) == -1) {
                    cerr << "waiting for child failed" << endl;
                }
                else {
                    cur->completed = true;
                }
                if (waitpid(childpid2, &wstatus, 0) == -1) {
                    cerr << "waiting for child failed" << endl;
                }
                else {
                    nxt->completed = true;
                }*/
            }
        }
    }

    /*
    childpid1 = fork();
    if (childpid1 == -1) {
        cerr << "can't fork" << endl;
    }
    else if (childpid1 == 0) {
        // child process

        write_pipe(pipefd[1]);
        close_pipe(pipefd[0], pipefd[1]);
        execvp(cur->cname, cur->arg_list);

        exit(0);
    }
    else {
        //parent process
        childpid2 = fork();
        if (childpid2 == -1) {
            cerr << "can't fork" << endl;
        }
        else if (childpid2 == 0) {
            //child process
            read_pipe(pipefd[0]);
            close_pipe(pipefd[0], pipefd[1]);
            execvp(nxt->cname, nxt->arg_list);

            exit(0);
        }
        else {
            //parent process
            close_pipe(pipefd[0], pipefd[1]);
            if (waitpid(childpid1, &wstatus, 0) == -1) {
                cerr << "waiting for child failed" << endl;
            }
            else {
                cur->completed = true;
            }
            if (waitpid(childpid2, &wstatus, 0) == -1) {
                cerr << "waiting for child failed" << endl;
            }
            else {
                nxt->completed = true;
            }

        }
    }*/
    return;
}
void do_fork(my_proc* cur) {
    //just do fork
    int childpid;
    int wstatus;
    //proc_ptr++;
    if (check_unknown(cur->cname)) {
        cur->completed = true;
        return;
    }
    cur->pid = fork();
    if (cur->pid == -1) {
        cerr << "can't fork" << endl;
    }
    else if (cur->pid == 0) {
        //child process
        execvp(cur->cname, cur->arg_list);
        //cout << "exec" << endl;
        exit(0);
    }
    else {
        //parent process
        if (waitpid(cur->pid, &wstatus, 0) == -1) {
            cerr << "failed to wait for child" << endl;
        }
        else {
            cur->completed = true;
        }
    }
    return;

}
void line_counter() {

    for (int i = 0; i < proc.size(); i++) {
        if (proc[i]->line_count >= 0) { // it must pipe with someone later
            proc[i]->line_count--;
        }
    }
    return;
}

void check_proc_pipe(my_proc* cur) {
    // check whether has a process is the current one's prev
    for (int i = 0; i < proc.size()-1; i++) {
        if (proc[i]->line_count == 0) { // it should pipe with this one
            cur->prev.push_back(proc[i]);
            proc[i]->next = cur;
            //do_pipe(proc[i], cur);
        }
    }
    return;
}

void exec_cmd() {

    int wstatus;
    // go through the proc queue from not executed place
    // find some processes can be executed in this time.
    for (int i = 0; i < proc.size(); i++) {
        DEBUG_BLOCK (
                     cout << "check process status: " << proc[i]->cname << endl;
                     );
        if (proc[i]->completed == true || proc[i]->pid != -1)
            continue;
        if (proc[i]->line_count < 0) {
            //no pipe
            //proc_ptr++;
            do_fork(proc[i]);

        }
        else if (proc[i]->line_count == 0){

            if (proc[i] == nullptr || proc[i]->next == nullptr) {
                cerr << "something error when construct process pipe: " << proc[i]->cname << endl;
            }
            //proc_ptr+=2;
            do_pipe(proc[i], proc[i]->next);
            for (int j = 0; j < proc[i]->next->prev.size(); j++) {
                DEBUG_BLOCK (
                             cout << "wait process pid: " << proc[i]->next->prev[j]->pid
                             << "(" << proc[i]->next->prev[j]->cname << ")" << endl;
                             );
                if (waitpid(proc[i]->next->prev[j]->pid, &wstatus, 0) == -1) {
                    cerr << "waiting for child failed" << endl;
                }
                else {
                    proc[i]->next->prev[j]->completed = true;
                }
            }
            if (waitpid(proc[i]->next->pid, &wstatus, 0) == -1) {
                cerr << "failed to wait for child" << endl;
            }
            else {
                proc[i]->next->completed = true;
            }

        }
    }

    return;
}

void read_cmd() {
    char* cur_cmd;
    char* next_cmd;

    while (cmd.size()) {
        cur_cmd = cmd.front();
        cmd.pop_front();

        DEBUG_BLOCK (
                     cout << "create proc: " << cur_cmd << endl;
                     );
        char* cname = (char*)malloc(sizeof(char)*100);
        strcpy(cname, cur_cmd);
        my_proc* cur = new my_proc(cname);
        proc.push_back(cur);
        if (proc_ptr == proc.end()) {
            proc_ptr = proc.end()-1;
        }

        if (cmd.size() != 0) {    //this block is used for reading pipes or arguments
            next_cmd = cmd.front();
            while (next_cmd[0] != '|') {  //check whether next is the current command's argument
                if (cur->arg_count >= MAX_ARGUMENTS-1) {   // last one is NULL
                    cerr << "reach argument number limit for command:" << cur_cmd << endl;
                    cmd.pop_front();
                }
                cur->arg_list[cur->arg_count++] = strdup(next_cmd);
                cur->arg_list[cur->arg_count] = NULL;
                DEBUG_BLOCK (
                         cout << "read argument: " << next_cmd << endl;
                         );

                cmd.pop_front();
                if (cmd.size() == 0) {
                    break;
                }
                next_cmd = cmd.front();
            }
            if (strcmp(next_cmd, "|") == 0) { //pipe to next
                cur->line_count = 1;
                cmd.pop_front();
            }
            else if (next_cmd[0] == '|') {  //number pipe
                next_cmd[0] = ' ';
                cur->line_count = atoi(next_cmd);

                DEBUG_BLOCK (
                             cout << "number pipe of this command: " << cur->line_count << endl;
                );
                cmd.pop_front();
            }

        }

        check_proc_pipe(cur);
        exec_cmd();
        line_counter();
    }

    return;
}


int main(int argc, char** argv, char** envp) {

    char* cur_cmd;

    //initial PATH environment varible
    setenv("PATH", "bin/:.", 1);
    while (1) {
        char lined_cmd[20000];

        cout << "% ";
        // get one line command
        if (!cin.getline(lined_cmd, 20000)) {
           break;
        }
        cur_cmd = strtok(lined_cmd, " ");
        //cout << cur_cmd << endl;
        do {
            //built-in command
            if (strcmp(cur_cmd, "exit") == 0) {
                return 0;
            }

            else if (strcmp(cur_cmd, "printenv") == 0) {
                char* arg = strtok(NULL, " ");
                if (cur_cmd == NULL) {
                    cerr << "error: need arguments for printenv" << endl;
                    break;
                }
                char* env = getenv(arg);
                if (env != NULL) {
                    cout << env << endl;
                }
                line_counter();

            }
            else if (strcmp(cur_cmd, "setenv") == 0) {
                char* arg1 = strtok(NULL, " ");

                if (arg1 == NULL) {
                    cerr << "error: need arguments for setenv" << endl;
                    break;
                }
                char* arg2 = strtok(NULL, " ");
                if (arg2 == NULL) {
                    cerr << "error: need arguments for setenv" << endl;
                    break;
                }
                //cout << "setenv " << arg1 << " " << arg2 << endl;
                setenv(arg1, arg2, 1);
                line_counter();

            }
            else {
                char* s = (char*)malloc(sizeof(char)*100);
                strcpy(s, cur_cmd);
                cmd.push_back(s);
            }
            cur_cmd = strtok(NULL, " ");

        }while (cur_cmd != NULL);

        read_cmd();

    }
    free(cur_cmd);

    return 0;
}




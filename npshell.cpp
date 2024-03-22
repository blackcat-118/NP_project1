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



deque<char*> cmd;
class my_proc {
public:
    my_proc(char* cname): cname(cname), next(nullptr), arg_list{'\0'}, line_count(-1) {}
    my_proc* next;
    deque<my_proc*> prev;
    char* cname;
    char arg_list[256];
    int line_count;

};
deque<my_proc*> proc;
deque<my_proc*>::iterator proc_ptr = proc.end();

void create_pipe(int* pipefd) {

    if (pipe(pipefd) < 0) {
        cerr << "pipe error" << endl;
    }

    return;
}

bool check_unknown(char* cmd_name) {
    /*if (access(strcat(getenv("PATH"), cmd_name), F_OK) == -1) {
        cerr << " Unknown command: [" << cmd_name << "]." << endl;
        return true;
    }*/
    if (access(cmd_name, F_OK) == -1) {
        cerr << " Unknown command: [" << cmd_name << "]." << endl;
        return true;
    }
    return false;
}

void read_pipe(int readfd, int writefd) {

    close(0);
    dup(readfd);
    close(readfd);
    close(writefd);

    return;
}
void write_pipe(int readfd, int writefd) {

    close(1);
    dup(writefd);
    close(readfd);
    close(writefd);

    return;
}
 /*
int do_fork(my_proc* cur) {
    cout << "do fork" << endl;

    int childpid1, childpid2;
    int wstatus;

    proc_ptr+=2;
    // check unknown command
    if (check_unknown(cur->cname)) {
        proc_ptr--;
        return;
    }
    if (check_unknown(nxt->cname)) {
        return;
    }
    create_pipe(&pipefd[0]);
    return fork();

    if ((childpid1 = fork()) == -1) {
        cerr << "can't fork" << endl;
    }
    else if (childpid1 == 0) {
        // child process
        write_pipe(pipefd[0], pipefd[1]);
        execlp(cur->cname, cur->arg_list, NULL);
    }
    else {
        // parent process
        if ((childpid2 = fork()) == -1) {
            cerr << "can't fork" << endl;
        }
        else if (childpid2 == 0) {
            // child process
            read_pipe(pipefd[0], pipefd[1]);
            execlp(nxt->cname, nxt->arg_list, NULL);
        }
        else {
            // parent process
            close(pipefd[0]);
            close(pipefd[1]);
            if (waitpid(childpid1, &wstatus, 0) == -1) {
                cerr << "waiting for child failed" << endl;
            }
        }
    }


    //cout << "read: " << pipefd[0] << "write: " << pipefd[1] << endl;

    return -1;
}*/

void do_pipe(my_proc* cur, my_proc* nxt) {
    cout << "do pipe" << endl;
    int pipefd[2];
    int childpid1, childpid2;
    int wstatus;
    proc_ptr+=2;
    // check unknown command
    if (check_unknown(cur->cname)) {
        proc_ptr--;
        return;
    }
    if (check_unknown(nxt->cname)) {
        return;
    }
    create_pipe(&pipefd[0]);

    childpid1 = fork();
    if (childpid1 == -1) {
        cerr << "can't fork" << endl;
    }
    else if (childpid1 == 0) {
        // child process
        write_pipe(pipefd[0], pipefd[1]);
        execlp(cur->cname, cur->arg_list, NULL);
    }
    else {
        //parent process
        childpid2 = fork();
        if (childpid2 == -1) {
            cerr << "can't fork" << endl;
        }
        else if (childpid2 == 0) {
            //child process
            read_pipe(pipefd[0], pipefd[1]);
            execlp(nxt->cname, nxt->arg_list, NULL);
        }
        else {
            //parent process
            if (waitpid(childpid1, &wstatus, 0) == -1) {
                cerr << "waiting for child failed" << endl;
            }

        }
    }
    return;
}
void do_fork(my_proc* cur) {
    //just do fork
    int childpid;
    int wstatus;
    proc_ptr++;
    if (check_unknown(cur->cname)) {
        proc_ptr--;
        return;
    }
    childpid = fork();
    if (childpid == -1) {
        cerr << "can't fork" << endl;
    }
    else if (childpid == 0) {
        //child process
        execlp(cur->cname, "ls", "-l", "bin", NULL);
    }
    else {
        //parent process
        if (waitpid(childpid, &wstatus, 0) == -1) {
            cerr << "failed to wait for child" << endl;
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

int check_proc_pipe(my_proc* cur) {
    // check whether has a process is the current one's prev
    for (int i = 0; i < proc.size(); i++) {
        if (proc[i]->line_count == 0) { // it should pipe with this one
            cur->prev.push_back(proc[i]);
            proc[i]->next = cur;
            do_pipe(proc[i], cur);
        }
    }
    return 0;
}

void exec_cmd() {
    cout << "cmd index in proc: " << proc_ptr-proc.begin() << endl;
    my_proc* cur = proc[proc_ptr - proc.begin()];
    if (cur->line_count < 0) {
        //no pipe
        do_fork(cur);
    }
    else {
        check_proc_pipe(cur);
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
                     cout << "debug->create proc: " << cur_cmd << endl;
                     );
        cout << "create proc: " << cur_cmd << endl;
        my_proc* cur = new my_proc(cur_cmd);
        proc.push_back(cur);
        if (proc_ptr == proc.end()) {
            proc_ptr = proc.end()-1;
        }

        if (cmd.size() != 0) {    //this block is used for reading pipes or arguments
            next_cmd = cmd.front();
            if (strcmp(next_cmd, "|") == 0) { //pipe to next
                cur->line_count = 1;
                cmd.pop_front();
            }
            else if (next_cmd[0] == '|') {  //number pipe
                cur_cmd[0] = 0;
                cur->line_count = atoi(cur_cmd);

                DEBUG_BLOCK (
                             cout << "number pipe of this command: " << cur->line_count << endl;
                );
                cmd.pop_front();
            }
            else if (next_cmd[0] == '-' || access(next_cmd, F_OK) != -1) {  //check whether next is the current command's argument
                strcat(cur->arg_list, next_cmd);
                strcat(cur->arg_list, " ");
                DEBUG_BLOCK (
                             cout << "read argument: " << next_cmd << endl;
                             );
                cmd.pop_front();

                while (cmd.size()) {  // read other arguments
                    next_cmd = cmd.front();
                    if (next_cmd[0] != '-' && access(next_cmd, F_OK) == -1) {
                        break;
                    }
                    strcat(cur->arg_list, next_cmd);
                    strcat(cur->arg_list, " ");
                    cmd.pop_front();
                }

            }
        }

        line_counter();
        exec_cmd();

    }

    return;
}

int main(int argc, char** argv, char** envp) {


    char* cur_cmd;

    //initial PATH environment varible
    //setenv("PATH", )
    while (1) {
        char lined_cmd[20000];

        cout << "% ";
        // get one line command
        if (!cin.getline(lined_cmd, 20000)) {
           break;
        }
        cur_cmd = strtok(lined_cmd, " ");
        do {
            //built-in command
            if (strcmp(cur_cmd, "exit") == 0) {
                return 0;
            }

            else if (strcmp(cur_cmd, "printenv") == 0) {
                cur_cmd = strtok(NULL, " ");
                if (cur_cmd == NULL) {
                    cerr << "error: need arguments for printenv" << endl;
                    break;
                }
                cout << getenv(cur_cmd) << endl;
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
                setenv(arg1, arg2, 1);

            }
            else {
                cmd.push_back(cur_cmd);
            }
            cur_cmd = strtok(NULL, " ");

        }while (cur_cmd != NULL);

        /*if (cmd.size() == 0) {
            break;
        }*/

        read_cmd();

        cout << "after read" << endl;
        for (int i = 0; i < cmd.size(); i++) {
            cout << cmd[i] << endl;
        }


    }


    return 0;
}




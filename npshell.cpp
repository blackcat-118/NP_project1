using namespace std;
#include <iostream>
#include <cstring>
#include <deque>
#include <vector>
#include <unistd.h>
#include <signal.h>

deque<char*> cmd;
class my_proc {
public:
    my_proc(char* cname): cname(cname), next(nullptr), line_count(-1) {}
    my_proc* next;
    char* cname;
    int line_count;

};
deque<my_proc*> proc;

void do_fork() {
    cout << "do fork" << endl;


    //cout << "read: " << pipefd[0] << "write: " << pipefd[1] << endl;

    //fork();


    return;
}

/*int* create_pipe() {

    int pipefd[2];

    if (pipe(pipefd) < 0) {
        cerr << "pipe error" << endl;
    }

    return &pipefd;
}*/

void close_pipe(vector<int> fn) {

    for (int i = 0; i < fn.size(); i++) {
        cout << "close " << fn[i] << endl;
        close(fn[i]);
    }

    return;
}

void read_cmd() {
    if (cmd.size() == 0) {
        return;
    }
    char* cur_cmd = cmd.front();
    char* next_cmd;
    cmd.pop_front();
    cout << "create proc: " << cur_cmd << endl;

    my_proc* cur = new my_proc(cur_cmd);
    proc.push_back(cur);

    if (cmd.size() == 0) {
        next_cmd = nullptr;
        execlp(cur_cmd, NULL); //here should fork!!
        return;
    }
    else {
        next_cmd = cmd.front();
    }

    if (strcmp(next_cmd, "|") == 0) { //pipe to next
        cur->line_count = 1;
    }
    else if (next_cmd[0] == '|') {  //number pipe
        cur_cmd[0] = 0;
        cur->line_count = atoi(cur_cmd);
    }
    else {                          //no pipe
        execlp(cur_cmd, NULL);
    }

    return;
}

int main(int argc, char** argv, char** envp) {

    char lined_cmd[20000];
    char* cur_cmd;
    while (1) {
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




using namespace std;
#include <iostream>
#include <cstring>
#include <deque>
#include <vector>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef DEBUG
#define DEBUG_BLOCK(statement)
#else
#define DEBUG_BLOCK(statement) do { statement } while (0)
#endif

#define MAX_ARGUMENTS 10

deque<char*> cmd;
class my_proc {
public:
    my_proc(char* cname): cname(cname), next(nullptr), arg_count(1), arg_list{cname, NULL},
    line_count(-1), readfd(0), writefd(1), completed(false), pid(-1), output_file{}, err(false) {}
    my_proc* next;
    deque<my_proc*> prev;
    char* cname;
    int arg_count;
    char* arg_list[10];
    int line_count;
    int readfd;
    int writefd;
    bool completed;
    int pid;
    char output_file[100];
    bool err;

};
deque<my_proc*> proc;
deque<my_proc*>::iterator proc_ptr = proc.end();
int proc_indx = 0;
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
    char* h = env;
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
    free(h);

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
void wrerr_pipe(int writefd) {

    close(1);
    close(2);
    dup(writefd);
    dup(writefd);
    return;
}
void close_pipe(int fd1, int fd2) {

    close(fd1);
    close(fd2);
    return;
}
void close_pipes(vector<int> fd) {

    for (int i = 0; i < fd.size(); i++) {
        //cout << "close: " << fd[i] << endl;
        close(fd[i]);
    }

    return;
}

void set_prev_completed(my_proc* p) {
    for (int i = 0; i < p->prev.size(); i++) {
        my_proc* p1 = p->prev[i];
        p1->completed = true;
        if (p1->pid != -1) {
            kill(p1->pid, 9);
        }
        set_prev_completed(p1);
    }
    return;
}
vector<int> used_pipe;
deque<my_proc*> exist_proc;

void do_fork(my_proc* p) {
    int pipefd[2];
    if (p->pid != -1) {
        return;
    }
    //cout << p->line_count << endl;
    if (p->line_count > 0) {
        bool flag = false;
        for (int i = 0; i < proc_indx; i++) {
            my_proc* p1 = proc[i];

            if (p1->line_count == p->line_count) {
                //pipe to the same line later
                //let they use one pipe
                p->readfd = p1->readfd;
                p->writefd = p1->writefd;
                flag = true;
                break;
            }
        }
        if (flag == false) {
            //cout << "create pipe" << endl;
            create_pipe(&pipefd[0]);
            p->readfd = pipefd[0];
            p->writefd = pipefd[1];
            used_pipe.push_back(pipefd[0]);
            used_pipe.push_back(pipefd[1]);
        }
    }
    int readfd, writefd;
    if (!p->prev.empty()) {
        readfd = p->prev.front()->readfd;
    }
    else {
        readfd = p->readfd;
    }
    writefd = p->writefd;
    //cout << p->cname << readfd << writefd << endl;
    p->pid = fork();
    exist_proc.push_back(p);
    if (p->pid == -1) {
        cerr << "can't fork" << endl;
    }
    else if (p->pid == 0) {
        //child process
        if (readfd != 0) {
            read_pipe(readfd);
        }
        if (p->err) {
            wrerr_pipe(writefd);
        }
        else if (writefd != 1) {
            write_pipe(writefd);
        }

        int fd = 0;
        if (strlen(p->output_file) != 0) {
            fd = open(p->output_file, O_TRUNC|O_WRONLY|O_CREAT, 0644);
            write_pipe(fd);
        }
        //cout << p->cname << endl;
        close_pipes(used_pipe);  //also close the pipe in different pipeline

        execvp(p->cname, p->arg_list);
        if (fd != 0)
            close(fd);
        exit(0);
    }

    return;
}

/*void do_fork(my_proc* p, int depth, int readfd, int writefd) {
    //just do fork
    int wstatus;
    int pipefd[2];

    my_proc* nxt = p->next;
    //cout << "cname: " << p->cname<<endl;
    if (nxt == nullptr) { // reach pipeline end
        if (check_unknown(p->cname)) {
            p->completed = true;
            set_prev_completed(p);
            return;
        }
        p->pid = fork();
        exist_proc.push_back(p);
        if (p->pid == -1) {
            cerr << "can't fork" << endl;
        }
        else if (p->pid == 0) {
            //child process
            if (readfd != 0) {
                read_pipe(readfd);
                //close(readfd);
            }
            if (writefd != 1) {
                //close(writefd);
            }
            int fd = 0;
            if (strlen(p->output_file) != 0) {
                fd = open(p->output_file, O_TRUNC|O_WRONLY|O_CREAT, 0644);
                write_pipe(fd);
            }
            //cout << p->cname << endl;
            close_pipes(used_pipe);

            //close_pipe(readfd, writefd);
            execvp(p->cname, p->arg_list);
            if (fd != 0)
                close(fd);
            exit(0);
        }
        else {

        }
    }
    else {   // check whether next process has other previous processes to merge
        if (check_unknown(nxt->cname)) {
            nxt->completed = true;
            set_prev_completed(nxt);
            return;
        }
        if (depth >= 256) {
            my_proc* pr = exist_proc.front();
            if (waitpid(pr->pid, &wstatus, 0) == -1) {
                cerr << "failed to wait for child" << endl;
            }
            else {
                pr->completed = true;
                exist_proc.pop_front();
            }
        }
        create_pipe(&pipefd[0]);
        used_pipe.push_back(pipefd[0]);
        used_pipe.push_back(pipefd[1]);

        //do_fork(nxt, depth+1, pipefd[0], pipefd[1]);
        for (int i = 0; i < nxt->prev.size(); i++) {
            my_proc* p1 = nxt->prev[i];
            /*if (i != 0) {
                //cout << "wait " << nxt->prev[i-1]->cname << endl;
                if (waitpid(nxt->prev[i-1]->pid, &wstatus, 0) == -1) {
                    cerr << "waiting for child failed" << endl;
                }
                else {
                    nxt->prev[i-1]->completed = true;
                }
            }

            p1->pid = fork();
            exist_proc.push_back(p1);
            if (p1->pid == -1) {
                cerr << "can't fork" << endl;
            }
            else if (p1->pid == 0) {
                // child process
                //cout << "read: " << readfd << "write: " << pipefd[1] << endl;
                if (readfd != 0) {
                    read_pipe(readfd);
                }
                if (p1->err) {
                    wrerr_pipe(pipefd[1]);
                }
                else {
                    write_pipe(pipefd[1]);
                }

                //close_pipe(pipefd[0], pipefd[1]);
                //cout << p1->cname << endl;
                close_pipes(used_pipe);

                execvp(p1->cname, p1->arg_list);

                exit(0);
            }
            else {
                //parent process

                if (nxt->pid < 0) {

                    do_fork(nxt, depth+1, pipefd[0], pipefd[1]);
                }
                /*if (waitpid(p1->pid, &wstatus, 0) == -1) {
                    cerr << "waiting for child failed" << endl;
                }
                else {
                   p1->completed = true;
                }*/

                /*if (i != 0) {
                    cout << "wait " << nxt->prev[i-1]->cname << endl;
                    if (waitpid(nxt->prev[i-1]->pid, &wstatus, 0) == -1) {
                        cerr << "waiting for child failed" << endl;
                    }
                    else {
                        nxt->prev[i-1]->completed = true;
                    }
                }
            }

        }
        close_pipe(pipefd[0], pipefd[1]);
    }

    return;

}*/

void line_counter() {

    for (int i = 0; i < proc.size(); i++) {
        proc[i]->line_count--;
        if (proc[i]->line_count >= 0) { // it must pipe with someone later

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
        }
    }
    return;
}
void exec_cmd() {
    int wstatus;
    for (proc_indx; proc_indx < proc.size(); proc_indx++) {
        my_proc* p = proc[proc_indx];
        //cout << i << " ";
        if (p->completed == true || p->pid != -1)
            continue;
        if (check_unknown(p->cname)) {
            p->completed = true;
            set_prev_completed(p);
            continue;
        }
        do_fork(p);

    }
    for (int i = 0; i < proc.size(); i++) {
         my_proc* p = proc[i];
        if (p->completed == true)
            continue;
        if (p->line_count <= 0) {  //it means p's next is ready
            //used_pipe.clear();

            my_proc* nxt = p->next;
            if (nxt == nullptr) { //if it doesn't have next
                if (waitpid(p->pid, &wstatus, 0) == -1) {
                    cerr << "failed to wait for child" << endl;
                }
                else {
                    p->completed = true;
                }
            }
            else {
                while (nxt) {     //if it has next
                    //my_proc* p1 = p;
                    if (nxt->line_count > 0) { //it means next's next is not ready
                        break;
                    }

                    if (nxt->next == nullptr) { //reach pipeline end
                        if (p->readfd != 0) {
                            close(p->readfd);
                        }
                        if (p->writefd != 1) {
                            close(p->writefd);
                        }
                        for (int a = 0; a < used_pipe.size(); a++) {
                            if (used_pipe[a] == p->readfd || used_pipe[a] == p->writefd) {
                                used_pipe.erase(used_pipe.begin()+a);
                                a--;
                            }
                        }
                        if (waitpid(p->pid, &wstatus, 0) == -1) {
                            cerr << "failed to wait for child" << endl;
                        }
                        else {
                            p->completed = true;
                        }
                        break;
                    }
                    nxt = nxt->next;
                }
            }
        }
    }
    return;
}
/*void exec_cmd() {

    int wstatus;
    // go through the proc queue from not executed place
    // find some processes can be executed in this time.
    for (int i = 0; i < proc.size(); i++) {
        DEBUG_BLOCK (
                     cout << "check process status: " << proc[i]->cname << endl;
                     );
        my_proc* p = proc[i];
        if (p->completed == true || p->pid != -1)
            continue;

        if (p->line_count <= 0) {  //it means p's next is ready
            used_pipe.clear();
            if (check_unknown(p->cname)) {
                p->completed = true;
                set_prev_completed(p);
                continue;
            }
            my_proc* nxt = p->next;
            if (nxt == nullptr) { //if it doesn't have next
                do_fork(p, 0, 0, 1);
                /*if (waitpid(p->pid, &wstatus, 0) == -1) {
                    cerr << "failed to wait for child" << endl;
                }
                else {
                    p->completed = true;
                }
            }
            else {
                while (nxt) {     //if it has next
                    if (nxt->line_count > 0) { //it means next's next is not ready
                        break;
                    }
                    if (check_unknown(nxt->cname)) {
                        nxt->completed = true;
                        set_prev_completed(nxt);
                        break;
                    }
                    if (nxt->next == nullptr) { //reach pipeline end
                        do_fork(p, 0, 0, 1);
                    }
                    nxt = nxt->next;
                }
            }
        }
        while (!exist_proc.empty()) {
            my_proc* pr = exist_proc.front();
            if (pr->completed == false) {
                if (waitpid(pr->pid, &wstatus, 0) == -1) {
                    cerr << "failed to wait for child" << endl;
                }
                else {
                    pr->completed = true;
                }

            }
            //cout << "pop: " << pr->cname << endl;
            exist_proc.pop_front();

        }
    }
    return;
}*/

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
            while (next_cmd[0] != '|' && next_cmd[0] != '!') {  //check whether next is the current command's argument or file redirection
                if (strcmp(next_cmd, ">") == 0) {  //file redirection
                    cmd.pop_front();
                    if (cmd.size() == 0) {
                        cerr << "File redirection need a filename after > operator" << endl;
                        break;
                    }
                    next_cmd = cmd.front();
                    strcpy(cur->output_file, next_cmd);
                    cmd.pop_front();
                }
                else {   // command arguments
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
                }

                if (cmd.size() == 0) {
                    break;
                }
                next_cmd = cmd.front();
            }
            if (strcmp(next_cmd, "|") == 0) { //pipe to next
                cur->line_count = 1;
                cmd.pop_front();
            }
            else if (strcmp(next_cmd, "!") == 0) {
                cur->err = true;
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
            else if (next_cmd[0] == '!') {  //number pipe
                next_cmd[0] = ' ';
                cur->err = true;
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
    setenv("PATH", "bin:.", 1);
    while (1) {
        char lined_cmd[20000];

        cout << "% ";
        // get one line command
        if (!cin.getline(lined_cmd, 20000)) {
           break;
        }
        cur_cmd = strtok(lined_cmd, " ");
        if (cur_cmd == NULL)
            continue;
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




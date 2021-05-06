#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <sys/wait.h>
#define SIGSTOP 19
using namespace std;

void ctrlZHandler(int sig_num) {
    // TODO: Add your implementation

    // we were told that pipe and IO commands wont be tested for receiving signals
    std::cout << "smash: got ctrl-Z" << endl;
    SmallShell& shell = SmallShell::getInstance();
    Command* running_cmd = shell.getRunningCommand();
    if (running_cmd == nullptr){
        return;
    }
    pid_t running_process = running_cmd->process_id;
//    if (SmallShell::getInstance().is_pipeline_command) {
//        return;
//    }
    if (waitpid(running_process, NULL, WNOHANG) == 0) { // if only the process is still alive (not zombie)
        shell.jobs_list->addJob(running_cmd, true);
        if(kill(running_process, SIGSTOP) == -1)
        {
            perror("smash error: kill failed");
            return;
        }
        shell.forceRunningCommand(nullptr);
        cout << "smash: process " << running_process << " was stopped" << endl;
    }
}

void ctrlCHandler(int sig_num) 
{
    // TODO: Add your implementation TOMER
    std::cout << "smash: got ctrl-C" << endl;
    SmallShell& shell = SmallShell::getInstance();
    Command* running_cmd = shell.getRunningCommand();
    if (running_cmd == nullptr)
    {
        return;//return if nothing?
    }
    pid_t running_process = running_cmd->process_id;
    if(kill(running_process, SIGKILL) == -1)
    {
        perror("smash error: kill failed");
        return;
    }
    shell.forceRunningCommand(nullptr);
    cout << "smash: process " << running_process << " was killed" << endl;//do even in have nothing?
}

void alarmHandler(int sig_num) 
{
    // TODO: Add your implementation
    std::cout <<endl<< "smash got an alarm" << endl;


}


#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <cctype>
#include <fstream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include "../skeleton_smash/wait.h"
#include <sys/wait.h>
#define PATH_MAX	260
#include <iomanip>
#include "Commands.h"

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif
const std::string WHITESPACE = " \n\r\t\f\v";
enum COMMAND_KEY_WORD {SHOWPID, PWD, CD, JOBS, KILL, FG, BG, QUIT, CAT, EXTERNAL};
static COMMAND_KEY_WORD getCommand(string& cmd){
    if (cmd == "showpid"){
        return SHOWPID;
    }
    if (cmd == "pwd"){
        return PWD;
    }
    if (cmd == "cd"){
        return CD;
    }
    if (cmd == "jobs"){
        return JOBS;
    }
    if (cmd == "kill"){
        return KILL;
    }
    if (cmd == "fg"){
        return FG;
    }
    if (cmd == "bg"){
        return BG;
    }
    if (cmd == "quit"){
        return QUIT;
    }
    if (cmd == "cat"){
        return CAT;
    }
    return EXTERNAL;
}
static bool isPipeCommand(int num_of_args, char** parsed_cmd_line){
    for (int i = 0; i < num_of_args + 1 ; i++) {
        if (((strcmp(parsed_cmd_line[i], "|") == 0 && (i != num_of_args) )|| strcmp(parsed_cmd_line[i], "|&") == 0)) {
            return true;
        }
    }
    return false;
}
static bool isRedirectionCommand(int num_of_args, char** parsed_cmd_line, const char* cmd_line) {
    return string::npos != string(cmd_line).find('>');
}
string _ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for(std::string s; iss >> s; ) {
        args[i] = (char*)malloc(s.length()+1);
        memset(args[i], 0, s.length()+1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command_str line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command_str line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

SmallShell::SmallShell() : prompt("smash> ") , previous_path(string()), jobs_list(new JobsList()), process_id(getpid()), running_command(
        nullptr){
// TODO: add your implementation
}
void SmallShell::updatePWD(const char *to_update) {
    previous_path = to_update;
}
SmallShell::~SmallShell() {
// TODO: add your implementation
}
static void ignoreBackgroundSign(char* temp_cmd_line, const char* cmd_line){
    strcpy(temp_cmd_line, cmd_line);
    _removeBackgroundSign(temp_cmd_line);
}
/**
* Creates and returns a pointer to Command class which matches the given command_str line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
    int line_length = strlen(cmd_line);
    char noBKSignCmdLine [line_length];
    ignoreBackgroundSign(noBKSignCmdLine,cmd_line);
    char ** parsed_command_line_no_BK = new char* [COMMAND_MAX_ARGS + 1];
    int num_of_args = _parseCommandLine(noBKSignCmdLine, parsed_command_line_no_BK) - 1;
    string first_word = parsed_command_line_no_BK[0];
    if (isRedirectionCommand(num_of_args, parsed_command_line_no_BK, cmd_line)){
        return new RedirectionCommand(cmd_line);
    }
    if (isPipeCommand(num_of_args, parsed_command_line_no_BK)) {
        return new PipeCommand(cmd_line);
    }
    /** handle Built-In Commands **/
    switch (getCommand(first_word)) {
        case SHOWPID :
            return  new ShowPidCommand(cmd_line);
        case PWD:
            return  new GetCurrDirCommand(cmd_line);
        case CD:
            return new ChangeDirCommand(cmd_line, parsed_command_line_no_BK, previous_path.c_str(), num_of_args);
        case JOBS:
            return new JobsCommand(cmd_line, jobs_list);
        case KILL:
            return new KillCommand(cmd_line, jobs_list);
        case FG:
            return new ForegroundCommand(cmd_line, jobs_list);
        case BG:
            return new BackgroundCommand(cmd_line, jobs_list);
        case QUIT:
            return new QuitCommand(cmd_line, jobs_list);
        case CAT:
            return new CatCommand(cmd_line);
        case EXTERNAL:
            return new ExternalCommand(cmd_line);
    }
    return nullptr;
}
void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    char **parsed_command_line = new char *[COMMAND_MAX_ARGS + 1]; //We know maximum number of args is 20
    int num_of_words_in_line = _parseCommandLine(cmd_line, parsed_command_line);
    int num_of_args = num_of_words_in_line - 1;
    if (cmd_line[0] == '\0') {
        return;
    }
    if (num_of_args != -1 && strcmp(parsed_command_line[0], "chprompt") == 0) {
        if (num_of_args >= 1) {
            prompt = strcat(parsed_command_line[1], "> ");
        } else if (num_of_args == 0) {
            prompt = "smash> ";
        }
        return;
    }
    // for example:
    Command *cmd = CreateCommand(cmd_line);
    if (cmd != nullptr) {
        forceRunningCommand(cmd); //?
        jobs_list->removeFinishedJobs();
        cmd->execute();
    }
    // Please note that you must fork smash process for some commands (e.g., isBuiltCommand commands....)
}

std::string SmallShell::getPrompt() {
    return prompt;
}

std::string SmallShell::getDirectoryFullPath() {
    char* path = new char [80];
    getcwd(path, sizeof(char)*80);
    string returned_path = path;
    return returned_path;
}

const char * SmallShell::getLastPWD() {
    return previous_path.c_str();
}

std::string SmallShell::getProcessID() {
    return to_string(process_id);
}

void SmallShell::forceRunningCommand(Command *cmd) {
    running_command = cmd;
}

Command *SmallShell::getRunningCommand() {
    return running_command;
}


void GetCurrDirCommand::execute() {
    char* full_path = new char [PATH_MAX];
    getcwd(full_path,PATH_MAX);
    cout << full_path << endl;
    delete [] full_path;
}

Command::Command(const char *cmd_line) : cmd_line(cmd_line), process_id(getpid()), BKSignRemoved(new char [strlen(cmd_line)]), is_pipe(false) {
    strcpy(BKSignRemoved, cmd_line);
    _removeBackgroundSign(BKSignRemoved);
    parsed_command_line = new char* [COMMAND_MAX_ARGS + 1];
    num_of_args = _parseCommandLine(BKSignRemoved, parsed_command_line) - 1;

}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {
    isBuiltCommand = true;
    SmallShell::getInstance().forceRunningCommand(this);
}

void ChangeDirCommand::execute() {
    if (num_of_args > 1) {
        std::cerr << "smash error: cd: too many arguments" << std::endl;
        return;
    }
    if (strcmp(path,"-") == 0) {
        if (strcmp(last_pwd, "") == 0) {
            std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
            return;
        }
        strcpy(path,last_pwd);
    }
    string previous_path(SmallShell::getInstance().getLastPWD());
    char temp_buff[PATH_MAX];
    string new_previous_path(getcwd(temp_buff, PATH_MAX));
    if (chdir(path) == -1) {
        perror("smash error: chdir failed");
        SmallShell::getInstance().updatePWD(previous_path.c_str());
        return;
    }
    SmallShell::getInstance().updatePWD(new_previous_path.c_str());
}

ChangeDirCommand::ChangeDirCommand(const char *cmdLine, char **parsed_cmd_line, const char *plastPwd,
                                   int num_of_args)
        : BuiltInCommand(cmdLine), last_pwd((SmallShell::getInstance()).getLastPWD()), path(parsed_cmd_line[1]), num_of_args(num_of_args){

}

JobsList::JobEntry::JobEntry(Command *command, bool stopped, time_t time)
        : command_str(command->cmd_line), seconds_elapsed(time), stopped(stopped), process_id(command->process_id), command(command){
    SmallShell::getInstance().jobs_list->removeFinishedJobs();
    int max=0;
    for (auto job = SmallShell::getInstance().jobs_list->jobs_list.begin();
         job != SmallShell::getInstance().jobs_list->jobs_list.end(); job++) {
        if (job->job_id > max) {
            max = job->job_id;
        }
    }
    job_id = max + 1;
}

void JobsList::JobEntry::print() {
    std::string stopped_flag = (stopped ? " (stopped)" : "");
    std::string suffix = " secs" + stopped_flag;
    cout << "[" << job_id << "] " << command_str << " : " << process_id << " "
         << difftime(time(nullptr), seconds_elapsed) << suffix << endl;
}

bool JobsList::JobEntry::operator>(const JobsList::JobEntry &other) const {
    return job_id > other.job_id;
}

bool JobsList::JobEntry::operator<(const JobsList::JobEntry &other) const {
    return job_id < other.job_id;
}

bool JobsList::JobEntry::operator!=(const JobsList::JobEntry &other) const {
    return job_id != other.job_id;
}

bool JobsList::JobEntry::operator==(const JobsList::JobEntry &other) const {
    return job_id == other.job_id;
}

void JobsList::JobEntry::stopOrResume() {
    stopped  = !stopped;
}

void JobsList::JobEntry::setStopped(bool val) {
    stopped = val;
}

void JobsList::removeFinishedJobs() {
    if( jobs_list.empty() ) {
        return;
    }
    auto job = jobs_list.begin();
    while ( job != jobs_list.end() ) {
        auto next_job =job;
        next_job++;
        int currentPid=job->process_id;
        if ( waitpid(currentPid, nullptr, WNOHANG) == -1 /* is finished*/ || waitpid(currentPid, nullptr, WNOHANG) == job->process_id /* is zombie*/ ) {
            jobs_list.remove(*job);
        }
        job = next_job;
    }
}

void JobsList::addJob(Command *cmd, bool isStopped) {
    removeFinishedJobs();
    JobEntry to_add(cmd, isStopped, time(nullptr) /*start the clock*/);
    jobs_list.push_back(to_add);
}

void JobsList::printJobsList() {
    this->jobs_list.sort();
    for (auto job = jobs_list.begin(); job != jobs_list.end() ; job++) {
        job->print();
    }
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for (auto job = jobs_list.begin(); job != jobs_list.end(); job++) {
        if (job->job_id == jobId) {
            return &(*job);
        }
    }
    return nullptr;
}

bool JobsList::exists(int job_id) {
    return getJobById(job_id) != nullptr;
}

void JobsList::removeJobById(int jobId) {
    if (!exists(jobId)){
        return;
    }
    jobs_list.remove(*(getJobById(jobId)));
}

JobsList::JobEntry *JobsList::getLastStoppedJob() {
    if (noStoppedJobsFound()){
        return nullptr;
    }
    int jobId = 0;
    for(auto & job : jobs_list){
        if(job.stopped){
            if (job.job_id > jobId) {
                jobId = job.job_id;
            }
        }
    }
    return getJobById(jobId);
}

bool JobsList::noStoppedJobsFound() {
    for(auto & job : jobs_list){
        if(job.stopped){
            return false;
        }
    }
    return true;
}

void JobsList::killAllJobs() {
    removeFinishedJobs();
    if(jobs_list.empty()){
        return;
    }
    for (auto & job : jobs_list) {
        kill(job.process_id, 9);
        cout << job.process_id << ": " << job.command_str << endl;
    }
}


JobsCommand::JobsCommand(const char *cmdLine, JobsList *jobs) : BuiltInCommand(cmdLine), jobs_list(jobs) {
    jobs_list->removeFinishedJobs(); //remove all finished jobs before printing the list
}

void JobsCommand::execute() {
    jobs_list->printJobsList();      // print
}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {

}

void ShowPidCommand::execute() {
    std::cout << "smash pid is " << SmallShell::getInstance().getProcessID() << endl;
//    std::cerr << ("smash pid is perororororor");
}

KillCommand::KillCommand(const char *cmdLine, JobsList *jobs) : BuiltInCommand(cmdLine), jobs_list(jobs) {

}
static bool isValidJobID(string num)
{
    for (int i = 0; i < num.length(); i++)
        if (isdigit(num[i]) == 0) {
            return false;
        }
    return true;
}
static bool isValidSigunm(string signum) {
    if (signum[0] != '-') {
        return false;
    }
    for (int i = 1; i < signum.length(); i++) {
        if (!isdigit(signum[i])) {
            return false;
        }
    }
    return true;
}
void KillCommand::execute() {
    jobs_list->removeFinishedJobs();
    string signum_str = parsed_command_line[1];
    string job_id_str = parsed_command_line[2];
    int signum = -atoi(parsed_command_line[1]);
    int job_id = atoi(parsed_command_line[2]);
    if (num_of_args != 2 || !isValidSigunm(signum_str) || !isValidJobID(job_id_str) || signum < 0) {
        cout<< "smash error: kill: invalid arguments" <<endl;
        return;
    }
    JobsList::JobEntry *job = jobs_list->getJobById(job_id);
    if (job == nullptr) {
        cout << "smash error: kill: job-id " + to_string(job_id) +  " does not exist" << endl;
        return;
    }
    if (kill(job->process_id, signum ) == -1) {
        perror("smash error: kill failed");
        return;
    }
    cout << "signal number " << signum << " was sent to pid " << job->process_id <<endl;
}

ForegroundCommand::ForegroundCommand(const char *cmdLine, JobsList *jobs)
        : BuiltInCommand(cmdLine), jobs(jobs) {

}

void ForegroundCommand::execute() {
    int job_id = 0;
    jobs->removeFinishedJobs();
    if (num_of_args > 1) {
        cout << "smash error: fg: invalid arguments" << endl;
        return;
    }
    // given the job id
    if (num_of_args == 1) {
        if (!isValidJobID(parsed_command_line[1])) {
            cout << "smash error: fg: invalid arguments" << endl;
            return;
        }
        job_id = atoi(parsed_command_line[1]);
    }
    if (num_of_args == 0) {
        if (jobs->jobs_list.empty()) { // if no arguments but jobs_;ist is empty
            cout << "smash error: fg: jobs list is empty" << endl;
            return;
        }
        for (auto & job : jobs->jobs_list) {
            if (job.job_id > job_id) {
                job_id = job.job_id;
            }
        }
    }
    if (!(jobs->exists(job_id))) {
        cout<<"smash error: fg: job-id " + std::to_string(job_id) + " does not exist"<<endl;
        return;
    }
    JobsList::JobEntry *job_to_force_run = jobs->getJobById(job_id);
    if (job_to_force_run->stopped) { // if process was stopped then send it signal to continue
        if (kill(job_to_force_run->process_id, SIGCONT) == -1) {
            perror("smash error: kill failed");
            return;
        }
        job_to_force_run->stopOrResume();
    }
    cout << job_to_force_run->command_str << " : " << job_to_force_run->process_id << endl;
    SmallShell::getInstance().forceRunningCommand(job_to_force_run->command);
    jobs->removeJobById(job_id);
    waitpid(job_to_force_run->process_id, NULL, WUNTRACED); /**The status of any child processes specified by pid that are stopped, and whose status has not yet been reported since they stopped, shall also be reported to the requesting process.**/
}
BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs)
        : BuiltInCommand(cmd_line), jobs(jobs) {

}


void BackgroundCommand::execute() {
    jobs->removeFinishedJobs();
    JobsList::JobEntry *job;
    if (num_of_args > 1) {
        cout << ("smash error: bg: invalid arguments") << endl;
        return;
    }
    if (num_of_args == 1) {
        if (!isValidJobID(parsed_command_line[1])) {
            cout << "smash error: fg: invalid arguments" << endl;
            return;
        }
        job = jobs->getJobById(atoi(parsed_command_line[1]));
        if (!job) {
            cout << "smash error: bg: job-id " + std::to_string(atoi(parsed_command_line[1])) + " does not exist"
                 << endl;
            return;
        }
        if (!(job->stopped)) {
            cout << "smash error: bg: job-id " + std::to_string(atoi(parsed_command_line[1])) +
                    " is already running in the background" << endl;
            return;
        }
    } else {
        if (jobs->jobs_list.empty()) {
            cout << "smash error: bg: jobs list is empty" << endl;
            return;
        }
        if (jobs->noStoppedJobsFound()) {
            cout << "smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }
        job = jobs->getLastStoppedJob();
    }
    cout << job->command_str << " : " << job->job_id << endl; //print and then send signal
    if (kill(job->process_id, SIGCONT) == -1) {
        perror("smash error: kill failed");
        return;
    }
    job->stopOrResume();
}
QuitCommand::QuitCommand(const char *cmdLine, JobsList *jobs) : BuiltInCommand(cmdLine), jobs(jobs) {

}

void QuitCommand::execute() {
    if (num_of_args >= 1 && strcmp(parsed_command_line[1], "kill") == 0) {
        jobs->removeFinishedJobs();
        cout << "smash: sending SIGKILL signal to " << jobs->jobs_list.size() << " jobs:"<<endl;
        jobs->killAllJobs();
        //TODO: tomer said we do not have to send kill signal to smash process, we should ask about it!
    }
    exit(1);
}

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {
    isBuiltCommand = false;
}

void ExternalCommand::execute() {
    pid_t pid = fork();
    bool fork_error = (pid == -1);
    bool son = (pid == 0);
    bool parent = !son;
    SmallShell& shell = SmallShell::getInstance();
    if (fork_error) {
        perror("smash error: fork failed");
        return;
    }
    if (son) { // new process
        if (!is_pipe) {
            setpgrp(); // in this way, a process receiving some signals does not affects the smash process.
        }
        if (execl("/bin/bash","/bin/bash","-c",BKSignRemoved, nullptr)== -1) {
            perror("smash error: execl failed");
        }
        exit(0);
    }
    if (parent) { //current process
        process_id = pid;
        if (_isBackgroundComamnd(cmd_line.c_str())) {
            SmallShell::getInstance().jobs_list->addJob(this, false);
        } else {
            SmallShell::getInstance().forceRunningCommand(this);
            waitpid(pid, nullptr, WUNTRACED);
        }
    }
}

RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line), coutbuf(std::cout.rdbuf()) {
    string trimmed_cmd_line(_trim(cmd_line));
    int position = trimmed_cmd_line.find('>'); //can not be std::string::npos
    int start_of_filename = position;
    if (position >= trimmed_cmd_line.length() - 1) {
        //TODO: Throw error if redirection is at end if cmd_line - that is there is no output file
//        cout << "smash error";
    }
    if (cmd_line[position + 1] == '>') {
        writing_way = OVERRIDING;
        start_of_filename++;
    } else {
        writing_way = APPENDING;
    }
    start_of_filename++;
    command_to_redirect = SmallShell::getInstance().CreateCommand(trimmed_cmd_line.substr(0, position).c_str());
    filename = _trim(trimmed_cmd_line.substr(start_of_filename, trimmed_cmd_line.length() - 1));
}
void RedirectionCommand::execute() {
    //TODO: check if open failed??
    std::ofstream new_out;
//     prepare(new_out);
//     command_to_redirect->execute();
//     cleanup();
    int duplicated_stdout=prepare(new_out);
    if (duplicated_stdout == -1) {
        return;
    }
    command_to_redirect->execute();
    cleanup(duplicated_stdout);
}

int RedirectionCommand::prepare(std::ofstream &new_output) {
//    std::cout.rdbuf(new_output.rdbuf());
//    if (writing_way == OVERRIDING){
//        new_output.open(filename);
//        std::cout.rdbuf(new_output.rdbuf());
//    } else {
//        new_output.open(filename, std::fstream::app);
//    }
    int duplicated_stdout = dup(1);
    int open_outcome;
    close(1);
    if (writing_way == APPENDING) {
        open_outcome = open(filename.c_str(), O_WRONLY| O_TRUNC | O_CREAT, 0666);
    }
    if (writing_way == OVERRIDING){
        open_outcome = open(filename.c_str(), O_WRONLY| O_APPEND | O_CREAT, 0666);
    }
    if (open_outcome == -1) {
        dup2(duplicated_stdout,1);
        perror("smash error: open failed");
        return -1;
    }
    return duplicated_stdout;
}
void RedirectionCommand::cleanup(int duplicated_stdout) {
//    std::cout.rdbuf(coutbuf);
    dup2(duplicated_stdout,1);
    close(duplicated_stdout);
}

PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line) {
    is_pipe = true;
    std::string trimmed_cmd_line(_trim(cmd_line));
    int pipe_position = trimmed_cmd_line.find('|');
    int start1 = 0;
    int end1 = pipe_position - 1;
    int start2 = pipe_position + 1;
    int end2 = trimmed_cmd_line.length() - 1;
    isAmpersandPipe = trimmed_cmd_line[pipe_position + 1] == '&';
    if (isAmpersandPipe){
        start2++;
    }
    if (pipe_position >= (_trim(cmd_line).length()) - 1) {
        //TODO: throw error
        cout << "smash error: illegal use of >" << endl;
    }
    command1 = SmallShell::getInstance().CreateCommand(trimmed_cmd_line.substr(start1, end1).c_str());
    command2 = SmallShell::getInstance().CreateCommand(trimmed_cmd_line.substr(start2, end2).c_str());
}

void PipeCommand::execute() {
    command1->isBuiltCommand ? builtInExecute() : externalExecute();
//    int separated_process = fork(); //is this fork necessary? no. but in this extra fork we can forget about retrieving the fd of the smash process to it's previous state
//    if (separated_process == -1) {  // note that this process should not be treated as "separated" process, meaning if this process gets signal then the smash process should also get the signal.
//        perror("smash error: fork failed");
//        return;
//    }
//    if (separated_process == 0) {
//        int my_pipe[2];
//        pipe(my_pipe);
//        int pipe_process_id = fork();
//        if (pipe_process_id == -1) {
//            perror("smash error: fork failed");
//            exit(0);
//        }
//        if (pipe_process_id == 0) {
//            if (isAmpersandPipe) {
//                dup2(my_pipe[1], 2);
//                close(my_pipe[0]);
//                close(my_pipe[1]);
//            } else {
//                dup2(my_pipe[1], 1);
//                close(my_pipe[0]);
//                close(my_pipe[1]);
//            }
//            command1->execute();
//            exit(1);
//        } else {
//            waitpid(pipe_process_id, nullptr, WUNTRACED);
//            dup2(my_pipe[0], 0);
//            close(my_pipe[0]);
//            close(my_pipe[1]);
//            command2->execute();
//            exit(1);
//        }
//    }
//    else {
//        SmallShell::getInstance().forceRunningCommand(this);
//        waitpid(separated_process, nullptr, WUNTRACED);
//        return;
//    }
}

void PipeCommand::externalExecute() {
    bool should_duplicate_stderr = (isAmpersandPipe);
    int pipe_process = fork(); // why is this fork necessary?
    if (pipe_process == -1) {
        perror("smash error: fork failed");
        return;
    }
    if (pipe_process == 0) {
        int my_pipe[2];
        pipe(my_pipe);
        int first_command_process = fork();
        if (first_command_process == -1) {
            perror("smash error: fork failed");
            exit(1);
        }
        if (first_command_process == 0) {
            if (should_duplicate_stderr) {
                dup2(my_pipe[1], 2);
                close(my_pipe[0]);
                close(my_pipe[1]);
            } else {
                dup2(my_pipe[1], 1);
                close(my_pipe[0]);
                close(my_pipe[1]);
            }
            command1->execute();
            exit(1);
        } else {
            waitpid(first_command_process, nullptr, WUNTRACED);
            dup2(my_pipe[0], 0);
            close(my_pipe[0]);
            close(my_pipe[1]);
            command2->execute();
            exit(1);
        }
    }
    else {
        SmallShell::getInstance().forceRunningCommand(this);
        waitpid(pipe_process    , nullptr, WUNTRACED);
        return;
    }
}

static int saveOriginalAndDuplicate(int pipe[2], bool should_duplicate_stderr){
    int original_out, original_err;
    if (should_duplicate_stderr) {
        original_err = dup(2); //duplicate stderr
        dup2(my_pipe[1], 2); // mt_pipe[1] is the new stderr
        return original_err;
    } else {
        original_out = dup(1); //duplicate stdout
        dup2(my_pipe[1], 1); // my_pipe[2] is the new stdout
        return original_out;
    }
}
static void cleanDuplication(int original, bool should_duplicate_stderr){
    int file_dist = should_duplicate_stderr ? 2 : 1;
    dup2(original, file_dist); // clean up and return original stderr

}
void PipeCommand::builtInExecute() {
    int my_pipe[2];
    pipe(my_pipe);
//   some commands, we won't be able to execute in a child process. For example : showpid cmd printing a different pid from expected (smash pid).
    int original = saveOriginalAndDuplicate(my_pipe, isAmpersandPipe);
    command1->execute();
    close(my_pipe[1]);
    cleanDuplication(original, isAmpersandPipe);
    int pid = fork();
    if (pid == -1) {
        perror("smash error: fork failed");
        return;
    }
    if (pid == 0) {
        setpgrp();
        dup2(my_pipe[0], 0); // my_pipe[0] is the new stdin
        close(my_pipe[0]);
        close(my_pipe[1]);
        command2->execute();
        exit(1);
    } else {
        waitpid(pid, nullptr, WUNTRACED);
        return;
    }
}
CatCommand::CatCommand(const char *cmdLine) : BuiltInCommand(cmdLine) {

}

void CatCommand::execute() {
    int fd;
    int len;
    char buffer;
    for (int i = 0; i < num_of_args; i++)
    {
        fd = open(parsed_command_line[i+1],O_RDONLY);
        if(fd == -1)
        {
            perror("smash error: open failed");
            return;
        }

        len = 1;
        while(len != 0)
        {
            len = read(fd,&buffer,1);
            if(len == -1)
            {
                perror("smash error: read failed");
                return;
            }
            write(1,&buffer,1);
        }
        std::cout<<std::endl;
        close(fd);

    }
}
#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <list>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
enum FileWritingWay { OVERRIDING, APPENDING};
class Command {
// TODO: Add your data members
public:
    bool isBuiltCommand;
    std::string cmd_line;
    pid_t process_id;
    int num_of_args;
    char** parsed_command_line;
    char* BKSignRemoved;
    Command(const char* cmd_line);
    virtual ~Command() = default;
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line);
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    explicit ExternalCommand(const char* cmd_line);
    virtual ~ExternalCommand() = default;
    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
    Command* command1;
    Command* command2;
    bool isAmpersandPipe;
public:
    explicit PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
    void externalExecute();
    void builtInExecute();
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
    FileWritingWay writing_way;
    std::streambuf* coutbuf;
    std::string filename;
    Command* command_to_redirect;
public:
    explicit RedirectionCommand(const char* cmd_line);
    virtual ~RedirectionCommand() {}
    void execute() override;
    int prepare(std::ofstream &new_output);
    void cleanup(int duplicated_stdout);
};

class ChangeDirCommand : public BuiltInCommand //needs public:?????
{
public:
    const char *last_pwd;
    char* path;
    int num_of_args;
    ChangeDirCommand(const char *cmdLine, char **parsed_cmd_line, const char *plastPwd,
                     int num_of_args);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line){};
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line);
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
    JobsList* jobs;
public:
    QuitCommand(const char *cmdLine, JobsList *jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};




class JobsList {
public:
    class JobEntry {
        // TODO: Add your data members
    public:
        int job_id;
        pid_t process_id;
        std::string command_str;
        Command* command;
        time_t seconds_elapsed;
        time_t time_to_kill;
        bool stopped;
        JobEntry(Command *command, bool stopped, time_t time, int duration = -1);
        ~JobEntry() = default;
        void print();
        void stopOrResume();
        void setStopped(bool val);
        bool operator>(const  JobEntry& other) const;
        bool operator<(const  JobEntry& other) const;
        bool operator!=(const JobEntry& other) const;
        bool operator==(const JobEntry& other) const;
    };
    // TODO: Add your data members
    int max_job_id;
public:
    JobsList() = default;
    ~JobsList() = default;
    void addJob(Command* cmd, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry * getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob();
    // TODO: Add extra methods or modify exisitng ones as needed
    bool noStoppedJobsFound();
    bool exists(int job_id);
    std::list<JobEntry> jobs_list;
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* jobs_list;
public:
    JobsCommand(const char *cmdLine, JobsList *jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* jobs_list;
public:
    KillCommand(const char *cmdLine, JobsList *jobs);
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* jobs;
public:
    ForegroundCommand(const char *cmdLine, JobsList *jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* jobs;
public:
    BackgroundCommand(const char *cmd_line, JobsList *jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class CatCommand : public BuiltInCommand {
public:
    explicit CatCommand(const char *cmdLine);
    virtual ~CatCommand() {}
    void execute() override;
};


class SmallShell {
private:
    // TODO: Add your data members
    std::string prompt;
    std::string previous_path;
    pid_t process_id;
    Command* running_command;
public:
    bool is_pipeline_command;
    std::string getPrompt();
    const char * getLastPWD();
    std::string getProcessID();
    void forceRunningCommand(Command *cmd);
    Command* getRunningCommand();
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)  = delete;
    SmallShell();
    // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    std::string getDirectoryFullPath();
    ~SmallShell();
    void executeCommand(const char* cmd_line);
    // TODO: add extra methods as needed
    void updatePWD(const char *to_update);
    JobsList* jobs_list;

};

#endif //SMASH_COMMAND_H_

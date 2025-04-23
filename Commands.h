// Ver: 10-4-2025


#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <cstring>
#include <list>
#include <memory>


using namespace std;

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define PATH_SIZE (4096)

class Command {
    // TODO: Add your data members
   string cmd_line;
protected:
    vector<string> arguments;
public:
    Command(const char *cmd_line);

    virtual ~Command();

    virtual void execute() = 0;

    string getCmdLine() const{
        return cmd_line;
    }

    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class ExternalCommand;

class JobsList {

public:
    class JobEntry {
    public:
        // TODO: Add your data members
        pid_t pid;
        int JobId;
        ExternalCommand* cmd;
        ~JobEntry() { //TODO check if you really need to delete cmd
            delete cmd;
        }

        JobEntry(ExternalCommand* the_command, int job_id, pid_t pid) : pid(pid), JobId(job_id), cmd(the_command) {}

    };

    // TODO: Add your data members
    JobsList();

    ~JobsList();

    void addJob(pid_t pid, ExternalCommand *cmd, bool isStopped = false);

    void printJobsList();

    int getNumOfJobs() const;

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry* getJobById(int jobId);

    void removeJobById(int jobId);

    int getMaxId();

    void printAllJobsForQUIT();

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);

    // TODO: Add extra methods or modify exisitng ones as needed
private:
    int MaxId;
    vector<JobEntry*> JobList;
    int number_of_jobs;
};



class SmallShell {
private:
    // TODO: Add your data members
    string prompt;
    char* Previous_Path;
    JobsList* Jobs;


    SmallShell();

public:
    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();

    void executeCommand(const char *cmd_line);

    void setPrompt (const string new_prompt){
        prompt = new_prompt;
    }

    void SetPreviousPath(char* newPath) {
        if(Previous_Path != nullptr){
            delete[] Previous_Path;
        }
        int prev_path_len = strlen(newPath);
        Previous_Path = new char[prev_path_len + 1];
        strcpy(Previous_Path, newPath);
    }

    char* GetPreviousPath(){
        return Previous_Path;
    }

    const string getPrompt (){
        return prompt;
    }

    // TODO: add extra methods as needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() {
    }
};

class ExternalCommand : public Command {
    JobsList *jobs_list;
    bool is_background;
    bool is_complex = false;

public:
    ExternalCommand(const char *cmd_line, JobsList *jobsList);

    virtual ~ExternalCommand() {
    }

    void execute() override;

//    void turnComplex(){
//        is_complex = true;
//    }

    bool isCommandComplex();

    bool isCommandBackground() const;
};


class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {
    }

    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {
    }

    void execute() override;
};

class DiskUsageCommand : public Command {
public:
    DiskUsageCommand(const char *cmd_line);

    virtual ~DiskUsageCommand() {
    }

    void execute() override;
};

class WhoAmICommand : public Command {
public:
    WhoAmICommand(const char *cmd_line);

    virtual ~WhoAmICommand() {
    }

    void execute() override;
};

class NetInfo : public Command {
    // TODO: Add your data members **BONUS: 10 Points**
public:
    NetInfo(const char *cmd_line);

    virtual ~NetInfo() {
    }

    void execute() override;
};

class ChangePromptCommand : public BuiltInCommand{
    string prompt;
    SmallShell *shell;
public:
    ChangePromptCommand(const char *cmd_line, SmallShell *shell);

    virtual ~ChangePromptCommand() = default;

    void execute() override;
};


class GetCurrentDirectory : public BuiltInCommand {
public:
    GetCurrentDirectory(const char *cmd_line);

    virtual ~GetCurrentDirectory() {
    }

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() = default;

    void execute() override;
    
};

class ChangeDirectoryCommand : public BuiltInCommand {
    string NewDir;

public:
    ChangeDirectoryCommand(const char* cmd_line, SmallShell *shell);

    void execute() override;

    virtual ~ChangeDirectoryCommand() = default;
};


class JobsCommand : public BuiltInCommand {
    JobsList* m_JobsList;
public:
    JobsCommand(const char* cmd_line, JobsList* Jobs);

    void execute() override;

    virtual ~JobsCommand() = default;
};

class fgCommand : public BuiltInCommand {
private:
    JobsList* Jobs;
    pid_t pid;
    int JobId;
public:
    fgCommand(const char *cmd_line, JobsList *Jobs);

    void execute() override;

    virtual ~fgCommand() = default;
};


class QuitCommand : public BuiltInCommand{
private:
    JobsList* Jobs;
public:
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() = default;

    void execute() override;
};



class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() {
    }

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() {
    }

    void execute() override;
};

class AliasCommand : public BuiltInCommand {
public:
    AliasCommand(const char *cmd_line);

    virtual ~AliasCommand() {
    }

    void execute() override;
};

class UnAliasCommand : public BuiltInCommand {
public:
    UnAliasCommand(const char *cmd_line);

    virtual ~UnAliasCommand() {
    }

    void execute() override;
};

class UnSetEnvCommand : public BuiltInCommand {
public:
    UnSetEnvCommand(const char *cmd_line);

    virtual ~UnSetEnvCommand() {
    }

    void execute() override;
};

class WatchProcCommand : public BuiltInCommand {
public:
    WatchProcCommand(const char *cmd_line);

    virtual ~WatchProcCommand() {
    }

    void execute() override;
};

#endif //SMASH_COMMAND_H_

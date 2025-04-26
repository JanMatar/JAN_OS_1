// Ver: 10-4-2025


#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <cstring>
#include <list>
#include <map>
#include <memory>
#include <set>


using namespace std;

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define PATH_SIZE (200)

class Command {
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
};

class ExternalCommand;

class JobsList {

public:
    class JobEntry {
    public:
        pid_t pid;
        int JobId;
        ExternalCommand* cmd;
        ~JobEntry() { //TODO check if you really need to delete cmd
//            delete cmd;
        }

        JobEntry(ExternalCommand* the_command, int job_id, pid_t pid) : pid(pid), JobId(job_id), cmd(the_command) {}

    };

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

private:
    int MaxId;
    vector<JobEntry*> job_entries_vec_in_jobslist;
    int number_of_jobs;
};



class SmallShell {
private:
    string prompt;
    char* Previous_Path;
    JobsList* Jobs;
    map<string, string> Aliases;


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

    map<string ,string>& getaliases();
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

};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() {
    }
};

class ExternalCommand : public Command {
    string original_cmd_line;
    JobsList *jobs_list;
    bool is_background = false;
    bool is_complex = false;

public:
    ExternalCommand(const char *cmd_line, JobsList *jobsList);

    virtual ~ExternalCommand() {
    }

    void execute() override;

    bool isCommandComplex();

    bool isCommandBackground() const;

    string getOriginalCmdLine() const{
        return original_cmd_line;
    }
};


class RedirectionCommand : public Command {
    SmallShell *shell;
    string left_side_command;
    string file_name;
    bool append;
    Command* command;
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char *cmd_line, SmallShell *shell);

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

    virtual ~ChangePromptCommand() {}

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

    virtual ~ShowPidCommand(){}

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

    virtual ~JobsCommand() {}
};

class fgCommand : public BuiltInCommand {
private:
    JobsList* Jobs;
    pid_t pid;
    int JobId;
public:
    fgCommand(const char *cmd_line, JobsList *Jobs);

    void execute() override;

    virtual ~fgCommand() {}
};


class QuitCommand : public BuiltInCommand{
private:
    JobsList* Jobs;
public:
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() {}

    void execute() override;
};



class KillCommand : public BuiltInCommand {
    JobsList* Jobs;
public:
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() {};
    void execute() override;
};


class AliasCommand : public BuiltInCommand {
    SmallShell* shell;
    string name;
    string command;
    set<string> reserved_in_bash = {"watchproc", "usetenv", "alias", "unalias", ">>",">", "|", "du", "whoami", "netinfo","pwd","cd", "jobs", "fg", "quit", "kill", "chprompt","showpid","listdir"};
public:
    AliasCommand(const char *cmd_line, SmallShell* shell);

    virtual ~AliasCommand() {
    }

    void execute() override;

    //for finding the end of the alias
    int findEndOfAlias(){
        return arguments[arguments.size() - 1].find_first_of("'");
    }
};

class UnAliasCommand : public BuiltInCommand {
    SmallShell* shell;
public:
    UnAliasCommand(const char *cmd_line, SmallShell* shell);

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

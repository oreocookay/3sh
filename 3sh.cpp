#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <cerrno>
#include <fstream>
#include <fcntl.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "3sh.h"

const std::vector<std::string> builtins = {"cd", "help", "exit", "history"};
const std::string homedir = getenv("HOME");
std::string prevdir;
std::vector<std::string> history_file_buf;
std::vector<std::string> session_buf;
int last_status = 0;

int main(int argc, char **argv)
{
    signal(SIGINT, sigint_handle); // listen for ^C
    read_history_file();
    cmd_loop();
    return 0;
}

void cmd_loop()
{
    while (true) {
        std::string line = read_line();
        sesh_buf_add(line);
        ParsedLine pl = parse_line(line);
        expand_special(pl);
        int status = execute(pl);
        last_status = status;
    }
}

std::vector<int(*)(std::vector<std::string>&)> builtin_func = {
    &cd,
    &help,
    &exit_sh,
    &history
};

std::string read_line()
{
    char *line = readline(get_prompt().c_str());
    if (line == NULL) { // EOF
        std::vector<std::string> _;
        exit_sh(_); // say goodbye
        exit(0);
    }
    return std::string(line);
}

Command split_line(const std::string& line)
{
    if (line.empty()) {
        // blank line; continue
        return {};
    }

    // split line by whitespace and group quoted text as a single argument
    Command args;
    std::string cur;
    bool in_quotes = false;
    for (char c : line) {
        if (c == '"') {
            in_quotes = !in_quotes;
        }
        else if (std::isspace(c) && !in_quotes) {
            if (!cur.empty()) {
                // expand $? to the last command's return status
                if (cur == "$?") {
                    cur = std::to_string(last_status);
                }
                args.push_back(cur);
                cur.clear();
            }
        }
        else {
            // expand ~ to homedir if not quoted
            if (!in_quotes && c == '~') {
                cur += homedir;
            }
            else {
                cur += c;
            }
        }
    }
    if (in_quotes) {
        std::cerr << "3sh: unmatched quote found\n";
        return {};
    }
    // handle final token
    if (!cur.empty()) {
        if (cur == "$?") {
            cur = std::to_string(last_status);
        }
        args.push_back(cur);
    }
    return args;
}

ParsedLine parse_line(const std::string& line)
{
    ParsedLine pl;

    // pipe and append found
    auto pipe_pos = line.find('|');
    auto app_pos = line.find(">>");
    if (pipe_pos != std::string::npos && app_pos != std::string::npos && pipe_pos < app_pos) {
        pl.type = LineType::PIPE_REDIRECT;
        Command cmd1 = split_line(line.substr(0, pipe_pos));
        Command cmd2 = split_line(line.substr(pipe_pos + 1, app_pos - pipe_pos - 1));
        Command path = split_line(line.substr(app_pos + 2));
        pl.commands.push_back(cmd1);
        pl.commands.push_back(cmd2);
        pl.commands.push_back(path);
        return pl;
    }
    // pipe and redirection found
    auto pos = line.find('>');
    if (pipe_pos != std::string::npos && pos != std::string::npos && pipe_pos < pos) {
        pl.type = LineType::PIPE_REDIRECT;
        Command cmd1 = split_line(line.substr(0, pipe_pos));
        Command cmd2 = split_line(line.substr(pipe_pos + 1, pos - pipe_pos -1));
        Command path = split_line(line.substr(pos + 1));
        pl.commands.push_back(cmd1);
        pl.commands.push_back(cmd2);
        pl.commands.push_back(path);
        return pl;
    }
    pos = line.find(">>");
    if (pos != std::string::npos) {
        // append found
        pl.type = LineType::APPEND;
        Command cmd = split_line(line.substr(0, pos));
        Command path = split_line(line.substr(pos + 2));
        pl.commands.push_back(cmd);
        pl.commands.push_back(path);
        return pl;
    }
    pos = line.find('>');
    if (pos != std::string::npos) {
        // redirection found
        pl.type = LineType::REDIRECT;
        Command cmd = split_line(line.substr(0, pos));
        Command path = split_line(line.substr(pos + 1));
        pl.commands.push_back(cmd);
        pl.commands.push_back(path);
        return pl;
    }
    pos = line.find('|');
    if (pos != std::string::npos) {
        // pipe found
        pl.type = LineType::PIPE;
        Command cmd1 = split_line(line.substr(0, pos));
        Command cmd2 = split_line(line.substr(pos + 1));
        pl.commands.push_back(cmd1);
        pl.commands.push_back(cmd2);
        return pl;
    }
    else {
        // no pipes or redirections
        pl.type = LineType::SIMPLE;
        pl.commands.push_back(split_line(line));
    }
    return pl;
}

int execute(const ParsedLine& pl)
{
    switch (pl.type) {
        case LineType::SIMPLE:
            return exec_simple(pl.commands[0]);
        case LineType::REDIRECT:
            return exec_redirect(pl.commands, false);
        case LineType::APPEND:
            return exec_redirect(pl.commands, true);
        case LineType::PIPE:
            return exec_pipe(pl.commands);
        case LineType::PIPE_REDIRECT:
            return exec_pipe_redirect(pl.commands, false);
        case LineType::PIPE_APPEND:
            return exec_pipe_redirect(pl.commands, true);
    }
    return -1;
}

int exec_simple(Command args)
{
    if (args.empty()) {
        // blank line; continue
        return 0;
    }
    // execute builtin command
    for (int i = 0; i < builtins.size(); i++) {
        if (builtins[i] == args[0]) {
            return (*builtin_func[i])(args);
        }
    }
    // execute other command
    std::vector<char*> argv;
    for (const auto& arg: args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "3sh: fork() error" << '\n';
        return 1;
    }
    else if (pid == 0) {
        // child process
        execvp(argv[0], argv.data());
        std::cerr << "3sh: command not found" << '\n';
        _exit(1);
    }
    else {
        // parent process
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            return WTERMSIG(status);
        }
    }
    return -1;
}

int exec_redirect(Pipeline cmds, bool append)
{
    Command cmd = cmds[0];
    Command path = cmds[1];
    
    if (cmd.empty() || path.empty()) {
        std::cerr << "3sh: redirect error\n";
        return 2;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // child process
        int fd = (append) ? open(path[0].c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644)
                          : open(path[0].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (fd == -1) {
            std::cerr << "3sh: redirect error\n";
            _exit(2);
        }
        // redirect stdout to file
        dup2(fd, STDOUT_FILENO);
        close(fd);

        std::vector<char*> argv;
        for (auto& arg: cmd) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        execvp(argv[0], argv.data());
        std::cerr << "3sh: exec() error\n";
        _exit(3);
    }
    else {
        // parent process
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            return WTERMSIG(status);
        }
    }
    return -1;
}

int exec_pipe(Pipeline cmds)
{
    int fd[2];
    if (pipe(fd) == -1) {
        std::cerr << "3sh: pipe() error\n";
        return -1;
    }

    pid_t pid1 = fork();
    
    if (pid1 == -1) {
        std::cerr << "3sh: fork() error\n";
        return 1;
    }
    if (pid1 == 0) {
        // command 1 child process
        std::vector<char*> argv;
        for (const auto& arg: cmds[0]) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        // connect the write end of the pipe to stdout
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        // execute command 1
        execvp(argv[0], argv.data());
        std::cerr << "3sh: exec() error\n";
        _exit(4);
    }

    int pid2 = fork();
    if (pid2 == -1) {
        std::cerr << "3sh: fork() error\n";
        return 1;
    }
    if (pid2 == 0) {
        // command 2 child process
        std::vector<char*> argv;
        for (const auto& arg: cmds[1]) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        // connect the read end of the pipe to stdin
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        // execute command 2
        execvp(argv[0], argv.data());
        std::cerr << "3sh: exec() error\n";
        _exit(5);
    }

    // close the parent process file descriptors
    close(fd[0]);
    close(fd[1]);

    // wait for the plumbing
    int status;
    waitpid(pid2, &status, 0);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return WTERMSIG(status);
    }
    return 0;
}

int exec_pipe_redirect(Pipeline cmds, bool append)
{
    int fd[2];
    if (pipe(fd) == -1) {
        std::cerr << "3sh: pipe() error\n";
        return 1;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        // send command 1 output to the pipe
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);

        std::vector<char*> argv;
        for (auto& arg: cmds[0]) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        // execute command 1
        execvp(argv[0], argv.data());
        std::cerr << "3sh: exec() error\n";
        _exit(6);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        std::cerr << "3sh: fork() error\n";
        return 1;
    }

    if (pid2 == 0) {
        // send command 2 output to the file
        int outfd = (append) ? open(cmds[2][0].c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644)
                             : open(cmds[2][0].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (outfd == -1) {
            std::cerr << "3sh: redirect error\n";
            _exit(7);
        }

        dup2(fd[0], STDIN_FILENO);
        dup2(outfd, STDOUT_FILENO);

        close(fd[0]);
        close(fd[1]);
        close(outfd);

        std::vector<char*> argv;
        for (auto& arg : cmds[1])
            argv.push_back(const_cast<char*>(arg.c_str()));
        argv.push_back(nullptr);

        // execute command 2
        execvp(argv[0], argv.data());
        _exit(8);
    }
    close(fd[0]);
    close(fd[1]);

    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);
    return 0;
}

void sigint_handle(int)
{
    // handle ^C; wipe the current line and get a new prompt
    write(STDOUT_FILENO, "\n", 1);
    rl_on_new_line();
    rl_replace_line("", 0);
    rl_redisplay();
}

int cd(Command& args)
{
    char prevdir_buf[1024];
    char cwd_buf[1024];
    std::string cwd; 

    // return home
    if (args.size() == 1) {
        if (prevdir.empty()) {
            prevdir = getcwd(prevdir_buf, sizeof(prevdir_buf));
        }
        chdir(homedir.c_str());
    }
    // return to previous directory
    else if (args[1] == "-") {
        if (prevdir.empty()) {
            std::cerr << "3sh: previous directory does not yet exist\n";
            return 3;
        }
        cwd = getcwd(cwd_buf, sizeof(cwd_buf));
        chdir(prevdir.c_str());
        prevdir = cwd;
    }
    
    // cd into another path
    else {
        cwd = getcwd(cwd_buf, sizeof(cwd_buf));
        // succeed
        if (chdir(args[1].c_str()) == 0) {
            prevdir = cwd;
        }
        else {
            // fail
            if (errno == EACCES) {
                std::cerr << "3sh: permission denied\n";
                return 4;
            }
            else {
                std::cerr << "3sh: no such directory" << '\n';
                return 5;
            }
        }
    }
    return 0;
}

int help(std::vector<std::string>& args)
{
    std::cout << "3sh: lightweight shell ⚡︎version 0.9" << '\n';
    std::cout << "usage: [command] [argument(s)]" << '\n';
    std::cout << "built-in commands: ";
    for (int i = 0; i < builtins.size() - 1; i++) {
        std::cout << builtins[i] << ", ";
    }
    std::cout << builtins[builtins.size() - 1] << '\n';
    return 0;
}

int exit_sh(std::vector<std::string>& args)
{
    std::cout << "3sh: <exiting>" << '\n';
    write_history_file();
    std::exit(0);
}

int history(std::vector<std::string>& args) {
    int i = 0;
    for (i = 0; i < history_file_buf.size(); i++) {
        std::cout << i+1 << ' ' << history_file_buf[i] << '\n';
    }
    for (auto& cmd: session_buf) {
        std::cout << i+1 << ' ' << cmd << '\n';
        i++;
    }
    return 0;
}

void sesh_buf_add(std::string line)
{
    if (!line.empty()) {
        session_buf.push_back(line);
        add_history(line.c_str());
    } 
}

void read_history_file()
{
    {
        // create history file if it doesn't exist
        std::ofstream hist_file(homedir + '/' + ".3sh_history", std::ios::out | std::ios::app);
        if (!hist_file.is_open()) {
            std::cerr << "3sh: could not create history file\n";
            exit(1);
        }
    }

    // read from the history file and initialize our history buffer
    std::string line;

    std::ifstream hist_file(homedir + '/' + ".3sh_history");
    if (hist_file.is_open()) {
        while (std::getline(hist_file, line)) {
            history_file_buf.push_back(line);
            add_history(line.c_str());
        }
        hist_file.close();
    }
    else {
        std::cerr << "3sh: could not read history file\n";
        exit(1);
    }
}

void write_history_file()
{
    std::ofstream hist_file(homedir + '/' + ".3sh_history", std::ios::out | std::ios::app);
    if (hist_file.is_open()) {
        for (auto& cmd: session_buf) {
            hist_file << cmd << '\n';
        }
        hist_file.close();
    }
    else {
        std::cerr << "3sh: could not write to history file\n";
        exit(1);
    }
}

void expand_special(ParsedLine& pl)
{
    for (auto& cmd: pl.commands) {
        expand_special(cmd);
    }
}

void expand_special(Command& cmd)
{
    if (cmd.empty()) {
        // blank line; continue
        return;
    }
    if (cmd[0] == "ls") {
        cmd.insert(cmd.begin() + 1, "--color=auto");
    }
    else if (cmd[0] == "grep") {
        cmd.insert(cmd.begin() + 1, "--color=auto");
    }
}

std::string get_prompt()
{
    char cwd_buf[1024];
    getcwd(cwd_buf, sizeof(cwd_buf));
    std::string cwd = cwd_buf;
    if (cwd == homedir) {
        return "~ 3sh> ";
    }
    if (cwd == "/") {
        return "/ 3sh> ";
    }
    int pos = cwd.find_last_of('/');
    std::string base = cwd.substr(pos + 1);
    return base + " 3sh> ";
}

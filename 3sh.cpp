#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unistd.h>
#include <cerrno>
#include <fstream>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include "3sh.h"

const std::vector<std::string> builtins = {"cd", "help", "exit", "history"};
const std::string homedir = getenv("HOME");
std::string prevdir;
std::vector<std::string> history_file_buf;
std::vector<std::string> session_buf;

int main(int argc, char **argv)
{
    signal(SIGINT, sigint_handle); // listen for ^C
    read_history_file();
    cmd_loop();
    return 0;
}

void cmd_loop()
{
    int status = 1;

    while (status) {
        std::string line = read_line();
        sesh_buf_add(line);
        expand_special(line);
        ParsedLine pl = parse_line(line);
        status = execute(pl);
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
                args.push_back(cur);
                cur.clear();
            }
        }
        else {
            cur += c;
        }
    }
    if (in_quotes) {
        std::cerr << "3sh: unmatched quote found\n";
        return {};
    }
    if (!cur.empty()) {
        args.push_back(cur);
    }
    return args;
}

ParsedLine parse_line(const std::string& line)
{
    ParsedLine pl;

    auto pos = line.find(">>");
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
    if (pl.type == LineType::SIMPLE) {
        return exec_simple(pl.commands[0]);
    }
    if (pl.type == LineType::REDIRECT) {
        return exec_redirect(pl.commands, false);
    }
    if (pl.type == LineType::APPEND) {
        return exec_redirect(pl.commands, true);
    }
    if (pl.type == LineType::PIPE) {
        return exec_pipe(pl.commands);
    }
    std::cerr << "3sh: parsed line type could not be determined\n";
    return 1;
}

int exec_simple(Command args)
{
    if (args.empty()) {
        // blank line; continue
        return 1;
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
    }
    else if (pid == 0) {
        // child process
        execvp(argv[0], argv.data());
        std::cerr << "3sh: command not found" << '\n';
        exit(1);
    }
    else {
        // parent process
        wait(nullptr);
    }
    return 1;
}

int exec_redirect(Pipeline cmds, bool append)
{
    Command cmd = cmds[0];
    Command path = cmds[1];
    
    if (cmd.empty() || path.empty()) {
        std::cerr << "3sh: redirect error\n";
        return 1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // child process
        int fd = (append) ? open(path[0].c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644)
                          : open(path[0].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (fd == -1) {
            std::cerr << "3sh: redirect error\n";
            exit(1);
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
        exit(1);
    }
    else {
        // parent process
        wait(nullptr);
    }
    return 1;
}

int exec_pipe(Pipeline cmds)
{
    int fd[2];
    if (pipe(fd) == -1) {
        std::cerr << "3sh: pipe() error\n";
        return 1;
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
    }

    // close the parent process file descriptors
    close(fd[0]);
    close(fd[1]);

    // wait for the plumbing
    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);
    return 1;
}

void sigint_handle(int)
{
    // handle ^C; wipe the current line and get a new prompt
    write(STDOUT_FILENO, "\n", 1);
    rl_on_new_line();
    rl_replace_line("", 0);
    rl_redisplay();
}

int cd(std::vector<std::string>& args)
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
            return 1;
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
            }
            else {
                std::cerr << "3sh: no such directory" << '\n';
            }
        }
    }
    return 1;
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
    return 1;
}

int exit_sh(std::vector<std::string>& args)
{
    std::cout << "3sh: <exiting>" << '\n';
    write_history_file();
    return 0;
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
    return 1;
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

void expand_special(std::string& line)
{
    // expand ~ into homedir
    for (int i = 0; i < line.size(); i++) {
        if (line[i] == '~') {
            line.replace(i, 1, homedir);
            i += homedir.size() - 1;
        }
    }

    // expand ls into ls --color=auto
    std::string ls_color = "ls --color=auto";
    auto pos = line.find("ls");
    if (pos != std::string::npos) {
        line.replace(pos, 2, ls_color);
    }

    // expand grep into ls --color=auto
    std::string grep_color = "grep --color=auto";
    pos = line.find("grep");
    if (pos != std::string::npos) {
        line.replace(pos, 4, grep_color);
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

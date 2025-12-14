#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unistd.h>
#include <cerrno>
#include <fstream>
#include <limits.h>
#include <readline/readline.h>
#include <readline/history.h>

// main function declarations
void cmd_loop();
std::string read_line();
std::vector<std::string> split_line(std::string line);
int execute(std::vector<std::string> args);
int launch_ps(std::vector<std::string> args);
void sigint_handle(int);

// prototypes for built-in shell commands
int cd(std::vector<std::string>& args);
int help(std::vector<std::string>& args);
int exit_sh(std::vector<std::string>& args);
int history(std::vector<std::string>& args);
void sesh_buf_add(std::string line);
void read_history_file();
void write_history_file();
void expand_special(std::string& line);
std::string get_prompt();

// globals
const std::vector<std::string> builtins = {"cd", "help", "exit", "history"};
const std::string homedir(getenv("HOME"));
char cwd_buf[PATH_MAX];
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
    std::string line;
    std::vector<std::string> args;
    int status = 1;

    while (status) {
        line = read_line();
        sesh_buf_add(line);
        expand_special(line);
        args = split_line(line);
        status = execute(args);
    }
}

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

std::vector<std::string> split_line(std::string line)
{
    std::istringstream iss(line);
    std::string arg;
    std::vector<std::string> args;

    while (iss >> arg) {
        args.push_back(arg);
    }

    return args;
}

// vector of function pointers to handle builtins
std::vector<int(*)(std::vector<std::string>&)> builtin_func = {
    &cd,
    &help,
    &exit_sh,
    &history
};

int execute(std::vector<std::string> args)
{
    if (args.empty()) {
        // continue
        return 1;
    }
    // execute builtin command
    for (int i = 0; i < builtins.size(); i++) {
        if (args[0] == builtins[i]) {
            return (*builtin_func[i])(args);
        }
    }
    // execute other command
    return launch_ps(args);
}

int launch_ps(std::vector<std::string> args)
{
    // convert our command args into a vector of char* for execvp() to handle
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);

    for (const std::string& s: args) {
        argv.push_back(const_cast<char*>(s.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // child process
        if (execvp(argv[0], argv.data()) == -1) {
            std::cerr << "3sh: command not found" << '\n';
        }
        exit(1);
    }
    else if (pid < 0) {
        // error forking
        std::cerr << "3sh: fork() error" << '\n';
    }
    else {
        // parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        }
        while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
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
    // return home
    if (args.size() == 1) {
        args.push_back(homedir);
    }

    if (chdir(args[1].c_str()) != 0) {
        if (errno == EACCES) {
            std::cerr << "3sh: permission denied\n";
        }
        else {
            std::cerr << "3sh: no such directory" << '\n';
        }
        // return -1;
    }
    return 1;
}

int help(std::vector<std::string>& args)
{
    int i;
    std::cout << "3sh: lightweight shell, version 0.9" << '\n';
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
}

std::string get_prompt()
{
    getcwd(cwd_buf, sizeof(cwd_buf));
    std::string cwd(cwd_buf);
    std::string base;
    if (cwd == homedir) {
        return "~ 3sh> ";
    }
    if (cwd == "/") {
        return "/ 3sh> ";
    }
    int pos = cwd.find_last_of('/');
    base = cwd.substr(pos + 1);
    return base + " 3sh> ";
}

// abstractions for parsing lines
typedef std::vector<std::string> Command;
typedef std::vector<Command> Pipeline;

enum LineType {
    SIMPLE,
    PIPE,
    REDIRECT
};

struct ParsedLine {
    LineType type;
    Pipeline commands;
};

// main functions
void cmd_loop();

std::string read_line();

std::vector<std::string> split_line(std::string line);

int launch_ps(const std::vector<std::string>& args);

void sigint_handle(int);

void sesh_buf_add(std::string line);

void read_history_file();

void write_history_file();

void expand_special(std::string& line);

ParsedLine parse_line(const std::string& line);

std::string get_prompt();

int execute(const ParsedLine& pl);

int exec_simple(Command args);

int exec_redirect(Pipeline cmds);

int exec_pipe(Pipeline cmds);

// built-in shell commands
int cd(std::vector<std::string>& args);

int help(std::vector<std::string>& args);

int exit_sh(std::vector<std::string>& args);

int history(std::vector<std::string>& args);

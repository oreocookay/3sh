#include <iostream>
#include <vector>
#define SH_RL_BUFSIZE 1024

void sh_loop();
char *sh_read_line();
char **sh_split_line();
void sh_execute();

int main(int argc, char **argv)
{
    return EXIT_SUCCESS;
}

void sh_loop()
{
    char *line;
    char **args;
    int status;

    do {
        std::cout << "> ";
        line = sh_read_line();
        args = sh_split_line(line);
        status = sh_execute(args);

        delete line;
        delete args;
    }
    while (status);
}

char *sh_read_line()
{
    int bufsize = SH_RL_BUFSIZE;
    std::vector<char> buf(bufsize);
    int pos = 0;
    int c;

    while (true) {
        // read a character
        cin >> c;

        // if we hit EOF, replace it with a null character and return
        if (c == EOF || c == '\n') {
            buf[pos] = '\0';
            return buf;
        }
        else {
            buf[pos] = c;
        }
        pos++;
}

char **sh_split_line()
{
}

void sh_execute()
{
}

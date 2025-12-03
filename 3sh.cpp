#include <iostream>

void sh_loop();
void sh_read_line();
void sh_execute();

int main(int argc, char **argv)
{
    return EXIT_SUCCESS;
}

void sh_loop(){
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


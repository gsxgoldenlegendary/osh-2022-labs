//
// Created by songxiao on 22-4-26.
//

#ifndef SHELL_MAIN_H
#define SHELL_MAIN_H

#include "bits/stdc++.h"
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <pwd.h>

#define BUFFER_SIZE 256
#define     UP      "\033[A"
#define     DOWN    "\033[B"

enum Status {
    RESULT_NORMAL,
    ERROR_FORK,
    ERROR_COMMAND,
    ERROR_WRONG_PARAMETER,
    ERROR_MISS_PARAMETER,
    ERROR_TOO_MANY_PARAMETER,
    ERROR_CD,
    ERROR_SYSTEM,
    ERROR_EXIT,
    ERROR_MANY_IN_OUT,
    ERROR_FILE_NOT_EXIST,
    ERROR_PIPE,
    ERROR_PIPE_MISS_PARAMETER,
    NO_INNER_COMMAND,
    ERROR_EXPORT
};

extern char commands[BUFFER_SIZE][BUFFER_SIZE];
extern std::vector<std::string> args;
extern std::vector<std::string> history;
#endif //SHELL_MAIN_H

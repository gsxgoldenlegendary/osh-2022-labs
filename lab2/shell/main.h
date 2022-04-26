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

// 内置的状态码
enum Status{
    RESULT_NORMAL,
    ERROR_FORK,
    ERROR_COMMAND,
    ERROR_WRONG_PARAMETER,
    ERROR_MISS_PARAMETER,
    ERROR_TOO_MANY_PARAMETER,
    ERROR_CD_MISS_PARAMETER,
    ERROR_CD,
    ERROR_CWD,
    ERROR_SYSTEM,

    /* 重定向的错误信息 */
    ERROR_MANY_IN,
    ERROR_MANY_OUT,
    ERROR_FILE_NOT_EXIST,

    /* 管道的错误信息 */
    ERROR_PIPE,
    ERROR_PIPE_MISS_PARAMETER,
    NO_INNER_COMMAND
};

extern std::vector<std::string> args;

#endif //SHELL_MAIN_H

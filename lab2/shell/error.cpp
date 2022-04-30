//
// Created by songxiao on 22-4-26.
//

#include "error.h"

bool error_process(Status e) {
    switch (e) {
        case RESULT_NORMAL:
            return true;
        case ERROR_WRONG_PARAMETER:
            std::cerr << "\e[31;1mError: No such path \"" << args[1] << "\".\n\e[0m";
            break;
        case ERROR_TOO_MANY_PARAMETER:
            std::cerr << "\e[31;1mError: Too many parameters while using command .\n\e[0m";
            break;
        case ERROR_FORK:
            std::cerr << "\e[31;1mError: Fork error.\n\e[0m";
            exit(ERROR_FORK);
        case ERROR_COMMAND:
            std::cerr << "\e[31;1mError: Command not exist in myshell.\n\e[0m";
            break;
        case ERROR_MANY_IN_OUT:
            std::cerr << "\e[31;1mError: Too many redirection symbols.\n\e[0m";
            break;
        case ERROR_FILE_NOT_EXIST:
            std::cerr << "\e[31;1mError: Input redirection file not exist.\n\e[0m";
            break;
        case ERROR_MISS_PARAMETER:
            std::cerr << "\e[31;1mError: Miss redirect file parameters.\n\e[0m";
            break;
        case ERROR_PIPE:
            std::cerr << "\e[31;1mError: Open pipe error.\n\e[0m";
            break;
        case ERROR_PIPE_MISS_PARAMETER:
            std::cerr << "\e[31;1mError: Miss pipe parameters.\n\e[0m";
            break;
        case ERROR_CD:
            std::cerr << "\e[31;1mcd failed.\n\e[0m";
            break;
        case ERROR_EXIT:
            exit(-1);
        case NO_INNER_COMMAND:
            return false;
        case ERROR_SYSTEM:
            std::cerr << "\e[31;1mError: System error while getting current work directory.\n\e[0m";
            exit(ERROR_SYSTEM);
        case ERROR_EXPORT:
            std::cerr << "\e[31;1export failed.\n\e[0m";
            break;
    }
    return true;
}
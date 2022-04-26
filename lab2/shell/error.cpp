//
// Created by songxiao on 22-4-26.
//

#include "error.h"
void error_process(Status e){
    switch (e) {
        default:;
        case RESULT_NORMAL:
            break;
        case ERROR_FORK:
            break;
        case ERROR_COMMAND:
            break;
        case ERROR_WRONG_PARAMETER:
            break;
        case ERROR_MISS_PARAMETER:
            break;
        case ERROR_TOO_MANY_PARAMETER:
            break;
        case ERROR_CD:
            break;
        case ERROR_SYSTEM:
            break;
        case ERROR_EXIT:
            break;
        case ERROR_MANY_IN:
            break;
        case ERROR_MANY_OUT:
            break;
        case ERROR_FILE_NOT_EXIST:
            break;
        case ERROR_PIPE:
            break;
        case ERROR_PIPE_MISS_PARAMETER:
            break;
    }
}
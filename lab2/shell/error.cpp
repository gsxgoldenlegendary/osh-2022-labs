//
// Created by songxiao on 22-4-26.
//

#include "error.h"
bool error_process(Status e){
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
        case ERROR_CD_MISS_PARAMETER:
            // 输出的信息尽量为英文，非英文输出（其实是非 ASCII 输出）在没有特别配置的情况下（特别是 Windows 下）会乱码
            // 如感兴趣可以自行搜索 GBK Unicode UTF-8 Codepage UTF-16 等进行学习
            std::cerr<<"\e[31;1mError: Miss parameter while using command \"cd\".\n\e[0m";
            // 不要用 std::endl，std::endl = "\n" + fflush(stdout)
            break;
        case ERROR_SYSTEM:
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
        case ERROR_CD:
            std::cerr<<"\e[31;1mcd failed.\n\e[0m";
            break;
        case ERROR_CWD:
            std::cerr<<"\e[31;1mcwd failed.\n\e[0m";
            break;
        case NO_INNER_COMMAND:
            return false;
            break;
    }
}
//
// Created by songxiao on 22-4-26.
//

#include "main.h"
#include "shell.h"
#include "error.h"

char commands[BUFFER_SIZE][BUFFER_SIZE];
std::vector<std::string> args;
std::vector<std::string> history;

void sighandler(int sig) {
    if (sig == SIGINT) {
        waitpid(0, nullptr, 0);
    }
    //输出提示符
    prepare();
}

//这是主函数，程序的主要部分
int main() {
    //处理Ctrl+C
    signal(SIGINT, sighandler);
    //读历史记录
    read_history();
    //无限循环
    while (true) {
        //输出提示符
        error_process(prepare());
        //有命令
        if (spilt_command()) {
            //执行内部指令
            if (!error_process(call_inner_command()))
                //执行外部指令
                error_process(call_outer_command());
        }
    }
    //一个好习惯
    return 0;
}
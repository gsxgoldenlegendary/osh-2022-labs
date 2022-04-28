//
// Created by songxiao on 22-4-26.
//

#include "main.h"
#include "shell.h"
#include "error.h"

char commands[BUFFER_SIZE][BUFFER_SIZE];
std::vector<std::string> args;

void sighandler(int sig){
    if(sig==SIGINT) {
        waitpid(0, nullptr, 0);
    }
}

//这是主函数，程序的主要部分
int main() {
    signal(SIGINT, sighandler);
    while (true) {
        //获取用户名、主机名及工作目录
        error_process(prepare());
        spilt_command();
        if (!args.empty()) { // 用户有输入指令
            if (!error_process(call_inner_command()))
                error_process(call_outer_command());
        }
    }
    //一个好习惯
    return 0;
}
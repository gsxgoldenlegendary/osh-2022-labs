//
// Created by songxiao on 22-4-26.
//

#include "main.h"
#include "shell.h"
#include "error.h"

char commands[BUFFER_SIZE][BUFFER_SIZE];

//这是主函数，程序的主要部分
int main() {
    while (true) {
        //获取用户名、主机名及工作目录
        error_process(prepare());
        int commandNum = spilt_command();
        if (commandNum != 0) { // 用户有输入指令
            if (strcmp(commands[0], "exit") == 0) { // exit命令
                error_process(callExit());
            } else if (strcmp(commands[0], "cd") == 0) { // cd命令
                error_process(callCd(commandNum));
            } else { // 其它命令
                error_process(call_outer_command(commandNum));
            }
        }
    }
    //一个好习惯
    return 0;
}
//
// Created by songxiao on 22-4-26.
//

#include "main.h"
#include "shell.h"
#include "error.h"

//命令行参数构成的容器
std::vector<std::string> args;

//这是主函数，程序的主要部分
int main() {
    // 不同步 iostream 和 cstdio 的 buffer
    std::ios::sync_with_stdio(false);
    //无限循环
    while (true) {
        //打印提示符并读取一行，分割命令
        error_process(prepare());
        // 有可处理的命令
        if (!args.empty()) {
            //处理内部指令
            error_process(call_inner_commands());
            //处理外部指令
            error_process(call_outer_commands());
        }
    }
    //一个好习惯
    return 0;
}
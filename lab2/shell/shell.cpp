//
// Created by songxiao on 22-4-26.
//
#include "shell.h"

//判断指令是否存在
bool command_exist(const std::string &command) {
    if (command.empty())
        return false;
    auto result = true;
    //创建管道
    int fds[2];
    if (pipe(fds) == -1) {
        result = false;
    } else {
        //获取标准输入输出的文件标识符
        auto inFd = dup(STDIN_FILENO);
        auto outFd = dup(STDOUT_FILENO);
        auto pid = vfork();
        if (pid == -1) {
            result = false;
        } else if (pid == 0) {
            //子进程通过管道将输出重定向给父进程
            close(fds[0]);
            dup2(fds[1], STDOUT_FILENO);
            close(fds[1]);
            //使用系统调用检查命令是否存在
            std::string tmp = "command -v " + command;
            system(tmp.c_str());
            exit(1);
        } else {
            waitpid(pid, nullptr, 0);
            close(fds[1]);
            dup2(fds[0], STDIN_FILENO);
            close(fds[0]);
            //如果子进程检查命令不存在，会返回EOF，在这里重定向为标准输入
            if (getchar() == EOF) {
                result = false;
            }
            //恢复标准输入输出
            dup2(inFd, STDIN_FILENO);
            dup2(outFd, STDOUT_FILENO);
        }
    }
    return result;
}

Status call_outer_command() {
    auto pid = fork();
    if (pid == -1) {
        return ERROR_FORK;
    } else if (pid == 0) {
        int inFds = dup(STDIN_FILENO);
        int outFds = dup(STDOUT_FILENO);
        //子进程执行可能含有管道的命令
        int result = call_pipe_command(0, args.size());
        dup2(inFds, STDIN_FILENO);
        dup2(outFds, STDOUT_FILENO);
        exit(result);
    } else {
        //父进程等待子进程执行结束，并根据status判断子进程执行结果
        int status;
        waitpid(pid, &status, 0);
        return Status(WEXITSTATUS(status));
    }
}

Status call_pipe_command(unsigned long head, unsigned long tail) {
    if (head >= tail)
        return RESULT_NORMAL;
    //找出管道位置，没有则为-1
    int pipeIdx = -1;
    for (auto i = head; i < tail; ++i) {
        if (args[i] == "|") {
            pipeIdx = int(i);
            break;
        }
    }
    if (pipeIdx == -1) {
        //没有管道，则处理重定向
        return call_redirect_command(head, tail);
    } else if (pipeIdx + 1 == tail) {
        return ERROR_PIPE_MISS_PARAMETER;
    }
    //有管道则建立管道
    int fds[2];
    if (pipe(fds) == -1) {
        return ERROR_PIPE;
    }
    int result = RESULT_NORMAL;
    auto pid = vfork();
    if (pid == -1) {
        result = ERROR_FORK;
    } else if (pid == 0) {
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        close(fds[1]);
        //子进程处理不含管道的前一半命令
        result = call_redirect_command(head, pipeIdx);
        exit(result);
    } else {
        //父进程等待子进程执行结束，并根据status判断子进程执行结果
        int status;
        waitpid(pid, &status, 0);
        auto exit_code = WEXITSTATUS(status);
        //子进程 未正常退出
        if (exit_code != RESULT_NORMAL) {
            std::string info;
            char line[BUFFER_SIZE];
            close(fds[1]);
            dup2(fds[0], STDIN_FILENO);
            close(fds[0]);
            while (fgets(line, BUFFER_SIZE, stdin) != nullptr) {
                info += line;
            }
            std::cout << info;
            result = exit_code;
        } else if (pipeIdx + 1 < tail) {
            //子进程正常执行，则重定向输入，递归执行后续可能含有管道的指令
            close(fds[1]);
            dup2(fds[0], STDIN_FILENO);
            close(fds[0]);
            result = call_pipe_command(pipeIdx + 1, tail);
        }
    }
    return Status(result);
}

Status call_redirect_command(unsigned long head, unsigned long tail) {
    if (!command_exist(args[head])) {
        return ERROR_COMMAND;
    }
    int input_number = 0;
    int output_number_w = 0;
    int output_number_a = 0;
    char *input_file = nullptr;
    char *output_file_w = nullptr;
    char *output_file_a = nullptr;
    auto endIdx = tail;
    for (auto i = head; i < tail; ++i) {
        if (strcmp(commands[i], "<") == 0) {
            ++input_number;
            if (i + 1 < tail)
                input_file = commands[i + 1];
            else return ERROR_MISS_PARAMETER;
            endIdx = i;
        } else if (strcmp(commands[i], ">") == 0) {
            ++output_number_w;
            if (i + 1 < tail)
                output_file_w = commands[i + 1];
            else return ERROR_MISS_PARAMETER;
            endIdx = i;
        } else if (strcmp(commands[i], ">>") == 0) {
            ++output_number_a;
            if (i + 1 < tail)
                output_file_a = commands[i + 1];
            else return ERROR_MISS_PARAMETER;
            endIdx = i;
        }
    }
    if (input_number == 1) {
        FILE *fp = fopen(input_file, "r");
        if (fp == nullptr)
            return ERROR_FILE_NOT_EXIST;
        fclose(fp);
    }
    if (input_number > 1 || output_number_w > 1 || output_number_a > 1) {
        return ERROR_MANY_IN_OUT;
    }
    auto result = RESULT_NORMAL;
    auto pid = vfork();
    if (pid == -1) {
        result = ERROR_FORK;
    } else if (pid == 0) {
        //子进程打开文件并进行系统调用
        if (input_number == 1)
            freopen(input_file, "r", stdin);
        if (output_number_w == 1)
            freopen(output_file_w, "w", stdout);
        else if (output_number_a == 1)
            freopen(output_file_a, "a", stdout);
        char *comm[BUFFER_SIZE];
        for (auto i = head; i < endIdx; ++i)
            comm[i] = commands[i];
        comm[endIdx] = nullptr;
        execvp(comm[head], comm + head);
        exit(errno);
    } else {
        //父进程等待子进程执行结束，并根据status判断子进程执行结果
        int status;
        waitpid(pid, &status, 0);
        int err = WEXITSTATUS(status);
        if (err) {
            std::cerr << "\e[31;1mError:" << strerror(err) << "\n\e[0m";
        }
    }
    return result;
}

Status prepare() {
    char hostname[BUFFER_SIZE];
    char curPath[BUFFER_SIZE];
    Status result;
    if (getcwd(curPath, BUFFER_SIZE) == nullptr)
        result = ERROR_SYSTEM;
    else
        result = RESULT_NORMAL;
    auto username = getpwuid(getuid())->pw_name;
    gethostname(hostname, BUFFER_SIZE);
    std::cout << "\e[32;1m" << username << "@" << hostname << ":" << curPath << "\e[0m$ ";
    std::cout.flush();
    return result;
}

bool spilt_command() {
    static unsigned long long command_number;
    command_number = history.size();
    std::string cmd;
    std::string input_buffer;
    int character;
    //取消缓冲区
    system("stty -icanon");
    while (true) {
        character = getchar();
        //遇到Ctrl-D直接退出
        if (character == 4) {
            std::ofstream outputer;
            outputer.open(".supershell");
            for (auto &&x: history) {
                outputer << x << std::endl;
            }
            outputer.close();
            pid_t pid = getpid();
            kill(pid, SIGTERM);
            //遇到换行符本轮输入结束
        } else if (character == '\n') {
            break;
        } else {
            input_buffer += char(character);
            if (input_buffer == UP) {
                //清除上箭头回显
                std::cout << "\b\b\b\b    \b\b\b\b";
                //第一条特殊处理
                if (command_number > 0) {
                    command_number--;
                    //清除上一条命令回显
                    if (command_number < history.size() - 1) {
                        for (auto i = 0; i < history[command_number + 1].size(); ++i)
                            std::cout << "\b";
                        for (auto i = 0; i < history[command_number + 1].size(); ++i)
                            std::cout << " ";
                        for (auto i = 0; i < history[command_number + 1].size(); ++i)
                            std::cout << "\b";
                    }
                    std::cout << history[command_number];
                }
                cmd = history[command_number];
                input_buffer = "";
            } else if (input_buffer == DOWN) {
                //清除下箭头回显
                std::cout << "\b\b\b\b    \b\b\b\b";
                if (command_number < history.size()) {
                    command_number++;
                    //清除上一条命令回显
                    if (command_number > 0) {
                        for (auto i = 0; i < history[command_number - 1].size(); ++i)
                            std::cout << "\b";
                        for (auto i = 0; i < history[command_number - 1].size(); ++i)
                            std::cout << " ";
                        for (auto i = 0; i < history[command_number - 1].size(); ++i)
                            std::cout << "\b";
                    }
                    //最后一条特殊处理
                    if (command_number != history.size())
                        std::cout << history[command_number];
                }
                if (command_number != history.size())
                    cmd = history[command_number];
                else
                    cmd = "";
                input_buffer = "";
            } else {
                cmd += input_buffer[input_buffer.size() - 1];
            }
        }
    }
    //切分命令，装入容器
    auto temp = split(cmd, " ");
    if (temp[0][0] == '!') {
        //找到!n命令
        unsigned long long n;
        //找到！！命令
        if (temp[0][1] == '!') {
            n = command_number - 1;
        } else {
            n = stoull(temp[0].substr(1));
        }
        temp = split(history[n], " ");
        std::cout << n << "\t" << history[n] << std::endl;
    }
    //找到history n命令
    if (temp[0] == "history") {
        if (cmd[0] != '!')
            history.push_back(cmd);
        unsigned long long n = stoull(temp[1]);
        if (n > command_number)
            n = command_number;
        for (auto i = command_number - n; i < command_number; ++i)
            std::cout << i << "\t" << history[i] << std::endl;
        return false;
    } else {
        if (cmd[0] != '!')
            history.push_back(cmd);
        args = temp;
    }
    for (auto i = 0; i < args.size(); ++i) {
        strcpy(commands[i], args[i].c_str());
    }
    return true;
}

Status call_inner_command() {
    if (strcmp(commands[0], "exit") == 0) { // exit命令
        std::ofstream outputer;
        outputer.open(".supershell");
        for (auto &&x: history) {
            outputer << x << std::endl;
        }
        outputer.close();
        pid_t pid = getpid();
        if (kill(pid, SIGTERM) == -1)
            return ERROR_EXIT;
        else
            return RESULT_NORMAL;
    } else if (strcmp(commands[0], "cd") == 0) { // cd命令
        if (args.size() <= 1) {
            // 输出的信息尽量为英文，非英文输出（其实是非 ASCII 输出）在没有特别配置的情况下（特别是 Windows 下）会乱码
            // 如感兴趣可以自行搜索 GBK Unicode UTF-8 Codepage UTF-16 等进行学习
            return ERROR_MISS_PARAMETER;
            // 不要用 std::endl，std::endl = "\n" + fflush(stdout)
        } else {
            // 调用系统 API
            int ret = chdir(commands[1]);
            if (ret)
                return ERROR_WRONG_PARAMETER;
        }
        return RESULT_NORMAL;
    } else if (strcmp(commands[0], "export") == 0) {
        for (auto i = ++args.begin(); i != args.end(); i++) {
            std::string key = *i;
            // std::string 默认为空
            std::string value;
            // std::string::npos = std::string end
            // std::string 不是 nullptr 结尾的，但确实会有一个结尾字符 npos
            size_t pos;
            if ((pos = i->find('=')) != std::string::npos) {
                key = i->substr(0, pos);
                value = i->substr(pos + 1);
            }
            int ret = setenv(key.c_str(), value.c_str(), 1);
            if (ret < 0) {
                return ERROR_EXPORT;
            }
        }
        return RESULT_NORMAL;
    } else
        return NO_INNER_COMMAND;
}

// 经典的 cpp string split 实现
// https://stackoverflow.com/a/14266139/11691878
std::vector<std::string> split(std::string s, const std::string &delimiter) {
    std::vector<std::string> res;
    size_t pos;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        res.push_back(token);
        s = s.substr(pos + delimiter.length());
    }
    res.push_back(s);
    return res;
}

void read_history() {
    std::ifstream inputer;
    //历史记录文件
    inputer.open(".supershell");
    if (inputer.is_open()) {
        std::string temp;
        while (getline(inputer, temp)) {
            history.push_back(temp);
        }
        inputer.close();
    }
}
//
// Created by songxiao on 22-4-26.
//
#include "shell.h"

int command_exist(const std::string &command) { // 判断指令是否存在
    if (command.empty())
        return false;
    int result = true;
    int fds[2];
    if (pipe(fds) == -1) {
        result = false;
    } else {
        /* 暂存输入输出重定向标志 */
        int inFd = dup(STDIN_FILENO);
        int outFd = dup(STDOUT_FILENO);
        pid_t pid = vfork();
        if (pid == -1) {
            result = false;
        } else if (pid == 0) {
            /* 将结果输出重定向到文件标识符 */
            close(fds[0]);
            dup2(fds[1], STDOUT_FILENO);
            close(fds[1]);
            std::string tmp = "command -v " + command;
            system(tmp.c_str());
            exit(1);
        } else {
            waitpid(pid, nullptr, 0);
            /* 输入重定向 */
            close(fds[1]);
            dup2(fds[0], STDIN_FILENO);
            close(fds[0]);
            if (getchar() == EOF) { // 没有数据，意味着命令不存在
                result = false;
            }
            /* 恢复输入、输出重定向 */
            dup2(inFd, STDIN_FILENO);
            dup2(outFd, STDOUT_FILENO);
        }
    }
    return result;
}

Status call_outer_command() { // 给用户使用的函数，用以执行用户输入的命令
    pid_t pid = fork();
    if (pid == -1) {
        return ERROR_FORK;
    } else if (pid == 0) {
        /* 获取标准输入、输出的文件标识符 */
        int inFds = dup(STDIN_FILENO);
        int outFds = dup(STDOUT_FILENO);
        int result = call_pipe_command(0, args.size());
        /* 还原标准输入、输出重定向 */
        dup2(inFds, STDIN_FILENO);
        dup2(outFds, STDOUT_FILENO);
        exit(result);
    } else {
        int status;
        waitpid(pid, &status, 0);
        return Status(WEXITSTATUS(status));
    }
}

Status call_pipe_command(unsigned long head, unsigned long tail) { // 所要执行的指令区间[head, tail)，可能含有管道
    if (head >= tail)
        return RESULT_NORMAL;
    /* 判断是否有管道命令 */
    int pipeIdx = -1;
    for (auto i = head; i < tail; ++i) {
        if (strcmp(commands[i], "|") == 0) {
            pipeIdx = int(i);
            break;
        }
    }
    if (pipeIdx == -1) { // 不含有管道命令
        return call_redirect_command(head, tail);
    } else if (pipeIdx + 1 == tail) { // 管道命令'|'后续没有指令，参数缺失
        return ERROR_PIPE_MISS_PARAMETER;
    }
    /* 执行命令 */
    int fds[2];
    if (pipe(fds) == -1) {
        return ERROR_PIPE;
    }
    int result = RESULT_NORMAL;
    pid_t pid = vfork();
    if (pid == -1) {
        result = ERROR_FORK;
    } else if (pid == 0) { // 子进程执行单个命令
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO); // 将标准输出重定向到fds[1]
        close(fds[1]);
        result = call_redirect_command(head, pipeIdx);
        exit(result);
    } else { // 父进程递归执行后续命令
        int status;
        waitpid(pid, &status, 0);
        int exitCode = WEXITSTATUS(status);
        if (exitCode != RESULT_NORMAL) { // 子进程的指令没有正常退出，打印错误信息
            char info[4096] = {0};
            char line[BUFFER_SIZE];
            close(fds[1]);
            dup2(fds[0], STDIN_FILENO); // 将标准输入重定向到fds[0]
            close(fds[0]);
            while (fgets(line, BUFFER_SIZE, stdin) != nullptr) { // 读取子进程的错误信息
                strcat(info, line);
            }
            printf("%s", info); // 打印错误信息
            result = exitCode;
        } else if (pipeIdx + 1 < tail) {
            close(fds[1]);
            dup2(fds[0], STDIN_FILENO); // 将标准输入重定向到fds[0]
            close(fds[0]);
            result = call_pipe_command(pipeIdx + 1, tail); // 递归执行后续指令
        }
    }
    return Status(result);
}

Status call_redirect_command(unsigned long head, unsigned long tail) {
    if (!command_exist(args[head])) {
        return ERROR_COMMAND;
    }
    int input_number = 0, output_number_w = 0, output_number_a = 0;
    char *input_file = nullptr, *output_file_w = nullptr, *output_file_a = nullptr;
    unsigned long endIdx = tail;
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
    /* 处理重定向 */
    if (input_number == 1) {
        FILE *fp = fopen(input_file, "r");
        if (fp == nullptr) // 输入重定向文件不存在
            return ERROR_FILE_NOT_EXIST;
        fclose(fp);
    }
    if (input_number > 1 || output_number_w > 1 || output_number_a > 1) {
        return ERROR_MANY_IN_OUT;
    }
    auto result = RESULT_NORMAL;
    pid_t pid = vfork();
    if (pid == -1) {
        result = ERROR_FORK;
    } else if (pid == 0) {
        /* 输入输出重定向 */
        if (input_number == 1)
            freopen(input_file, "r", stdin);
        if (output_number_w == 1)
            freopen(output_file_w, "w", stdout);
        else if (output_number_a == 1)
            freopen(output_file_a, "a", stdout);
        /* 执行命令 */
        char *comm[BUFFER_SIZE];
        for (auto i = head; i < endIdx; ++i)
            comm[i] = commands[i];
        comm[endIdx] = nullptr;
        execvp(comm[head], comm + head);
        exit(errno); // 执行出错，返回errno
    } else {
        int status;
        waitpid(pid, &status, 0);
        int err = WEXITSTATUS(status); // 读取子进程的返回码
        if (err) { // 返回码不为0，意味着子进程执行出错，用红色字体打印出错信息
            printf("\e[31;1mError: %s\n\e[0m", strerror(err));
        }
    }
    return result;
}

Status prepare() {
    char username[BUFFER_SIZE];
    char hostname[BUFFER_SIZE];
    char curPath[BUFFER_SIZE];
    Status result;
    // 获取当前工作目录
    if (getcwd(curPath, BUFFER_SIZE) == nullptr)
        result = ERROR_SYSTEM;
    else
        result = RESULT_NORMAL;
    // 获取当前登录的用户名
    auto pwd = getpwuid(getuid());
    strcpy(username, pwd->pw_name);
    // 获取主机名
    gethostname(hostname, BUFFER_SIZE);
    std::cout << "\e[32;1m" << username << "@" << hostname << ":" << curPath << "\e[0m$ ";
    std::cout.flush();
    return result;
}

void spilt_command() {
    std::string cmd;
    std::getline(std::cin, cmd);
    args = split(cmd, " ");
    for (auto i = 0; i < args.size(); ++i) {
        strcpy(commands[i], args[i].c_str());
    }
}

Status call_inner_command() {
    if (strcmp(commands[0], "exit") == 0) { // exit命令
        // std::string 转 int
        std::stringstream code_stream(commands[1]);
        int code = 0;
        code_stream >> code;
        // 转换失败
        if (!code_stream.eof() || code_stream.fail())
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
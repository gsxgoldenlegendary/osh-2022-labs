//
// Created by songxiao on 22-4-26.
//
#include "shell.h"

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

Status prepare() {
    //当前目录
    char current_path[BUFFER_SIZE];
    //主机名
    char hostname[BUFFER_SIZE];
    //用户名
    char username[BUFFER_SIZE];
    // 用来存储读入的一行命令
    std::string cmd;
    // 获取当前工作目录
    if (getcwd(current_path, BUFFER_SIZE) == nullptr) {
        std::cerr << "\e[31;1mError: System error while getting current work directory.\n\e[0m";
        exit(ERROR_SYSTEM);
    }
    // 获取当前登录的用户名
    strcpy(username, getpwuid(getuid())->pw_name);
    //获取主机名
    gethostname(hostname, BUFFER_SIZE);
    //打印提示符
    std::cout << "\e[32;1m" << username << "@" << hostname << ":" << current_path << "\e[0m#";
    // 读入一行。std::getline 结果不包含换行符。
    std::getline(std::cin, cmd);
    // 按空格分割命令为单词
    args = split(cmd, " ");
}

int call_redirect_command(int head, unsigned tail) {
    // 判断是否有重定向
    int inNum = 0, outNum0 = 0,outNum1=0;
    //char *inFile = nullptr, *outFile = nullptr;
    std::string inFile, outFile;
    auto endIdx = tail; // 指令在重定向前的终止下标

    for (int i = head; i < tail; ++i) {
        if (args[i] == "<") { // 输入重定向
            ++inNum;
            if (i + 1 < tail)
                inFile = args[i + 1];
            else return ERROR_MISS_PARAMETER; // 重定向符号后缺少文件名
            endIdx = i;
        } else if (args[i] == ">") { // 输出重定向
            ++outNum0;
            if (i + 1 < tail)
                outFile = args[i + 1];
            else return ERROR_MISS_PARAMETER; // 重定向符号后缺少文件名
            endIdx = i;
        }
        else if (args[i] == ">>") { // 输出追加重定向
            ++outNum1;
            if (i + 1 < tail)
                outFile = args[i + 1];
            else return ERROR_MISS_PARAMETER; // 重定向符号后缺少文件名
            endIdx = i;
        }
    }
    /* 处理重定向 */
    if (inNum == 1) {
        FILE *fp = fopen(inFile.c_str(), "r");
        if (fp == nullptr) // 输入重定向文件不存在
            return ERROR_FILE_NOT_EXIST;
        fclose(fp);
    }

    if (inNum > 1) { // 输入重定向符超过一个
        return ERROR_MANY_IN;
    } else if (outNum0 > 1||outNum1>1) { // 输出重定向符超过一个
        return ERROR_MANY_OUT;
    }

    int result = RESULT_NORMAL;
    pid_t pid = vfork();
    if (pid == -1) {
        result = ERROR_FORK;
    } else if (pid == 0) {
        /* 输入输出重定向 */
        if (inNum == 1)
            freopen(inFile.c_str(), "r", stdin);
        if (outNum0 == 1)
            freopen(outFile.c_str(), "w", stdout);
        else if (outNum1 == 1)
            freopen(outFile.c_str(), "a", stdout);

        /* 执行命令 */
        // std::vector<std::string> 转 char **
        char *arg_ptrs[endIdx - head + 1];
        for (auto i = head; i < endIdx; i++) {
            arg_ptrs[i] = &args[i][0];
        }
        // exec p 系列的 argv 需要以 nullptr 结尾
        arg_ptrs[endIdx - head] = nullptr;

        // execvp 会完全更换子进程接下来的代码，所以正常情况下 execvp 之后这里的代码就没意义了
        // 如果 execvp 之后的代码被运行了，那就是 execvp 出问题了
//        for (int i = head; i < endIdx; ++i)
//            std::cout << args[i] << " " << getpid() << std::endl;
        execvp(arg_ptrs[head], arg_ptrs+head);

        exit(errno); // 执行出错，返回errno
    } else {
        int status;
        waitpid(pid, &status, 0);
        int err = WEXITSTATUS(status); // 读取子进程的返回码
        if (err) { // 返回码不为0，意味着子进程执行出错，用红色字体打印出错信息
//            for (int i = head; i < endIdx; ++i)
//                std::cout << args[i] << " " << getpid() << std::endl;
            std::cerr << "\e[31;1mError: " << strerror(err) << "\n\e[0m";
        }
    }
    return result;
}

int call_pipe_command(int head, unsigned tail) {
    if (head >= tail)
        return RESULT_NORMAL;
    // 判断是否有管道命令
    int pipe_index = -1;
    for (auto i = head; i < tail; ++i) {
        if (args[i] == "|") {
            pipe_index = i;
            break;
        }
    }
    if (pipe_index == -1) { // 不含有管道命令
        return call_redirect_command(head, tail);
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
        result = call_redirect_command(head, pipe_index);
        exit(result);
    } else { // 父进程递归执行后续命令
        int status;
        waitpid(pid, &status, 0);
        int exitCode = WEXITSTATUS(status);

        if (exitCode != RESULT_NORMAL) { // 子进程的指令没有正常退出，打印错误信息
            std::string info;
            char line[BUFFER_SIZE];
            close(fds[1]);
            dup2(fds[0], STDIN_FILENO); // 将标准输入重定向到fds[0]
            close(fds[0]);
            while (fgets(line, BUFFER_SIZE, stdin) != nullptr) { // 读取子进程的错误信息
                info+=line;
            }
            // 打印错误信息
            std::cout << info;
            result = exitCode;
        } else if (pipe_index + 1 < tail) {
            close(fds[1]);
            dup2(fds[0], STDIN_FILENO); // 将标准输入重定向到fds[0]
            close(fds[0]);
            result = call_pipe_command(pipe_index + 1, tail); // 递归执行后续指令
        }
    }
    return result;
}

Status call_outer_commands() {
    // 外部命令
    pid_t pid = fork();
    if (pid == -1) {
        return ERROR_FORK;
    }
    if (pid == 0) {
        // 这里只有子进程才会进入
        //获取标准输入、输出的文件标识符
        int inFds = dup(STDIN_FILENO);
        int outFds = dup(STDOUT_FILENO);
        int result = call_pipe_command(0, args.size());
        // 还原标准输入、输出重定向
        dup2(inFds, STDIN_FILENO);
        dup2(outFds, STDOUT_FILENO);
        exit(result);
    }
    // 这里只有父进程（原进程）才会进入
    int ret = wait(nullptr);
    if (ret < 0) {
        return ERROR_WAIT;
    }
}

Status call_inner_commands() {
    if (args[0] == "exit") {    // 退出
        if (args.size() <= 1) {
            exit(0);
        }
        // std::string 转 int
        std::stringstream code_stream(args[1]);
        int code = 0;
        code_stream >> code;
        // 转换失败
        if (!code_stream.eof() || code_stream.fail()) {
            std::cout << "Invalid exit code\n";
            exit(EOF);
        }
        exit(code);
    } else if (args[0] == "cd") {// 更改工作目录为目标目录
        if (args.size() <= 1) {
            return ERROR_MISS_PARAMETER;
        }
        // 调用系统应用程序接口
        int ret = chdir(args[1].c_str());
        if (ret < 0) {
            return ERROR_CD;
        }
        return INNER_COMMAND;
    } else if (args[0] == "pwd") {// 显示当前工作目录
        std::string cwd;
        // 预先分配好空间
        cwd.resize(PATH_MAX);
        // std::string to char *: &s[0]（C++17 以上可以用 s.data()）
        // std::string 保证其内存是连续的
        const char *ret = getcwd(&cwd[0], PATH_MAX);
        if (ret == nullptr) {
            std::cout << "cwd failed\n";
        } else {
            std::cout << ret << std::endl;
        }
        return INNER_COMMAND;
    } else if (args[0] == "export") {// 设置环境变量
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
                std::cout << "export failed\n";
            }
        }
        return INNER_COMMAND;
    }
    return NO_INNER_COMMAND;
}
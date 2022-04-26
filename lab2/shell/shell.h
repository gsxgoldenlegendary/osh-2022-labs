//
// Created by songxiao on 22-4-26.
//

#ifndef SHELL_SHELL_H
#define SHELL_SHELL_H
#include "main.h"

std::vector<std::string> split(std::string, const std::string &);

Status prepare();

int call_redirect_command(int, unsigned);

int call_pipe_command(int, unsigned);

Status call_outer_commands();

Status call_inner_commands();


#endif //SHELL_SHELL_H

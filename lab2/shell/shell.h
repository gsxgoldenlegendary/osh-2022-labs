//
// Created by songxiao on 22-4-26.
//

#ifndef SHELL_SHELL_H
#define SHELL_SHELL_H

#include "main.h"

int command_exist(const std::string&);

Status call_outer_command();

Status call_pipe_command(unsigned long, unsigned long);

Status call_redirect_command(unsigned long, unsigned long);

Status prepare();

void spilt_command();

Status call_inner_command();

std::vector<std::string> split(std::string, const std::string &);

#endif //SHELL_SHELL_H

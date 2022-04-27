//
// Created by songxiao on 22-4-26.
//

#ifndef SHELL_SHELL_H
#define SHELL_SHELL_H

#include "main.h"

int command_exist(const char *);

Status callExit();

Status call_outer_command(int);

Status call_pipe_command(int, int);

Status call_redirect_command(int, int);

Status callCd(int);

Status prepare();

int spilt_command();

#endif //SHELL_SHELL_H

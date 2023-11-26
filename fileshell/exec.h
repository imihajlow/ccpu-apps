#pragma once

#include <stdint.h>
#include <stdbool.h>

bool exec(const char *filename);

bool exec_fd(uint8_t fd);

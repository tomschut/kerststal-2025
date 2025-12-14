#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <tuple>

#ifndef MAIN_UTIL_CPP
#define MAIN_UTIL_CPP

void wait(int milliseconds) { vTaskDelay(pdMS_TO_TICKS(milliseconds)); }
std::tuple<int, int, int> makeColor(int r, int g, int b) { return std::make_tuple(r, g, b); }
#endif // MAIN_UTIL_CPP

#include "fmt/core.h"
#include <cstdlib>
#include <fmt/format.h>
#include <iostream>

#define source_info fmt::format("{}:{}", __FILE__, __LINE__)

#define logi(x, ...)                                                                                                   \
    std::cout << fmt::format("{:<5} {:<25} ", "INFO", source_info) << fmt::format(x, ##__VA_ARGS__) << std::endl

#define logd(x, ...)                                                                                                   \
    std::cout << fmt::format("{:<5} {:<25} ", "DEBUG", source_info) << fmt::format(x, ##__VA_ARGS__) << std::endl

// #define loge(x, ...)                                                                                                   \
//     std::cout << fmt::format("{:<5} {:<25} ", "ERROR", source) << fmt::format(x, ##__VA_ARGS__) << std::endl;          \
//     exit(-1)
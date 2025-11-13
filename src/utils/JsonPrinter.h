#pragma once
#include <nlohmann/json.hpp>
#include <iostream>

inline void printJson(const nlohmann::json& j) {
    std::cout << j.dump(4) << std::endl;
}
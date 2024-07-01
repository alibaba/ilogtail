//
// Created by 韩呈杰 on 2023/10/25.
//
#include "common/ModuleTool.hpp"

// GCC下纯虚析构也是需要函数体的
IHandler::~IHandler() = default;

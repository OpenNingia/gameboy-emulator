#pragma once

#include <quill/LogMacros.h>
#include <quill/Logger.h>

namespace gbemu::log {
    void init();
    quill::Logger* root();
} // namespace gbemu::log

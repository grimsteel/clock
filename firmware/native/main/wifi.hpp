#pragma once

#if __cpp_exceptions

#include "esp_err.h"
#include "esp_exception.hpp"

namespace wifi {

struct WifiException : public idf::ESPException {
    WifiException(esp_err_t err) : idf::ESPException(err) {}
};

void connect();
void disconnect();

}

#endif

#include <string>
#include "Error.h"

Error::Error(const Coordinate& coord, const char* err) {
    msg = std::to_string(coord.line) + ":" + std::to_string(coord.pos) + ":" + err;
}

Error::Error(const Coordinate& coord, const std::string& err) {
    msg = std::to_string(coord.line) + ":" + std::to_string(coord.pos) + ":" + err;
}

const char* Error::what() {
    return msg.c_str();
}


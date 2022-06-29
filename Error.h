#pragma once

#include "Coordinate.h"


class Error : public std::exception {
public:
	Error(const Coordinate& coord, const char* err);

	Error(const Coordinate& coord, const std::string& err);

	const char* what();
private:
	std::string msg;
};


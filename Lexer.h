#pragma once

#include <vector>
#include <random>
#include <string>

#include "Coordinate.h"
#include "Defines.h"
#include "Error.h"


class Lexer {
private:
    Position current;

    bool get_attribute(std::string &);

    std::vector<Token> next();

    std::vector<Token> parse_sum_lower_bound(std::string s);

    std::vector<Token> parse_sum_upper_bound(std::string s);

    bool isSum = false;
    bool isProduct = false;
    bool isPlaceholder = false;
    bool isFloor = false;
    bool isCeil = false;

    std::vector<Token> sum_tokens;
    std::vector<Token> sum_iter_tokens;

    std::vector<Token> product_tokens;
    std::vector<Token> product_iter_tokens;

    std::string sumName;
    std::string productName;

public:
    Lexer();

    Lexer(const Position& p, const ProgramString&);

    ~Lexer();

    std::vector<Token> program_to_tokens(const ProgramString&);
};
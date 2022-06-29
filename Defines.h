#pragma once

#include <iostream>
#include <map>
#include <cmath>
#include <utility>
#include <vector>
#include <array>


enum Tag {
    NONE = 0,
    UADD, USUB, ADD, SUB, DIV, MUL, FRAC, POW,
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    SET, COMMA, BREAK, INDEX, AMP,
    WHILE, IF, ELSE, WHEN, OTHERWISE, ALT,
    BEGIN, END,
    BEGINB, ENDB, BEGINC, ENDC, BEGINM, ENDM,
    LT, GT, LEQ, GEQ, NEQ, EQ,
    OR, AND, NOT,
    NUMBER, IDENT, KEYWORD, FUNC,
    ERROR, SPACE,
    PLACEHOLDER, TEXT, LIST, ROOT,
    GRAPHIC, RANGE, TRANSP, SUM, PRODUCT, DIMENSION, SKIP, ABS, FLOOR, CEIL
};

typedef struct Tag_info {
    std::string name = "NONE";
    int priority = 0;
    Tag close_tag = NONE;
    Tag alternative_tag = NONE;
    bool is_operator = false;
    bool is_binary = false;
    bool is_inverted = false;

    explicit Tag_info(
        std::string = "NONE",
        int = 0,
        Tag = NONE,
        Tag = NONE,
        bool = false,
        bool = false,
        bool = false
    );
} Tag_info;


extern std::map<Tag, Tag_info> t_info ;

extern std::map<std::string, enum Tag> raw_tag;

extern std::map<std::string, enum Tag> dim_tag;

/**
 * m, kg, s, A, K, mol, cd
 */
extern std::map<std::string, std::array<int, 7>> const dimensions;

extern std::map<std::string, int> arg_count;

extern std::map<std::string, double> constants;

extern std::map<std::string, double (*)(double)> funcs1;

extern std::map<std::string, double (*)(double, double)> funcs2;

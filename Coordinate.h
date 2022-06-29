#pragma once

#include <string>
#include <iostream>
#include <utility>
#include "Defines.h"


typedef struct Coordinate {
    size_t line, pos;   //строка, смещение

    Coordinate(size_t = 1, size_t = 1);

    Coordinate &inc_line();

    Coordinate &inc_pos();

    Coordinate(const Coordinate &c);

    Coordinate &operator=(const Coordinate &c);

    bool operator==(const Coordinate &other) const;

    bool operator<(const Coordinate &other) const;

    friend std::string to_string(const Coordinate& c);
} Coordinate;

typedef struct ProgramString {
    std::string program;
    Coordinate begin;
    Coordinate end;
    size_t length = 0;
} ProgramString;

typedef struct Position {
    Coordinate start;
    size_t index;
    static ProgramString ps;
    enum cur_type {
        CHAR, NLINE, WNLINE
    };

    Position(const Position &p);

    Position(const Coordinate& = Coordinate(), size_t = 0);

    Position &operator=(const Position &p);

    int is_at_newline();

    Position &operator++();

    Position operator++(int);

    char operator[](int i) const;

    char &operator[](int i);

    char cur();

    bool can_peek(int = 1);

    char peek(int = 1);

    char get();

    bool operator<(const Position &p) const;

    bool operator==(const Position &p) const;

    friend std::string to_string(const Position& p);

    bool end_of_program() const;
} Position;


typedef struct Token {
    Position start; //координаты начала и конца
    Position end;
    std::string raw;    //подстрока из файла
    std::string _ident;    //строковое представление тега - для принта
    Tag _tag = ERROR;

    Token(const Token &t);

    Token(const Position&, const Position&, Tag = ERROR, std::string = "");

    Token &operator=(const Token &t);

    void convert();

    void unary();

    void binary();

    bool operator<(const Token &t) const;

    bool operator==(const Token &t) const;

    friend std::string to_string(const Position& p);
} Token;


std::string to_string(const Coordinate& c);

std::string to_string(const ProgramString& ps);

std::string to_string(const Position& p);

std::string to_string(const Token& l);
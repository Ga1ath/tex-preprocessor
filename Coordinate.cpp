#include <string>
#include <iostream>
#include <utility>
#include "Coordinate.h"


Coordinate::Coordinate(size_t l, size_t p) : line(l), pos(p) {}

Coordinate& Coordinate::inc_line() {
    line++;
    pos = 1;
    return *this;
}

Coordinate& Coordinate::inc_pos() {
    pos++;
    return *this;
}

Coordinate::Coordinate(const Coordinate &c) : line(c.line), pos(c.pos) {}

Coordinate& Coordinate::operator=(const Coordinate &c) {
    if (&c != this) {
        line = c.line;
        pos = c.pos;
    }
    return *this;
}

bool Coordinate::operator==(const Coordinate &other) const {
    return (line == other.line) && (pos == other.pos);
}

bool Coordinate::operator<(const Coordinate &other) const {
    return (line < other.line) || ((line == other.line) && (pos < other.pos));
}


Position::Position(const Position &p) : start(p.start), index(p.index) {}

Position::Position(const Coordinate& x, size_t i) : start(x), index(i) {
    if (ps.end < start) {
        std::cout << "Position:: ps.end < start\n";
        throw std::exception();
    }
}

Position& Position::operator=(const Position &p) {
    if (&p != this) {
        start = p.start;
        index = p.index;
    }
    return *this;
}

Position Position::operator++(int) {
    Position tmp(*this);
    operator++();
    return tmp;
}

char Position::operator[](int i) const {
    return ps.program[i];
}

char& Position::operator[](int i) {
    return ps.program[i];
}

char Position::cur() {
    return ps.program[index];
}

bool Position::can_peek(int i) {
    return index + i < ps.length;
}

char Position::peek(int i) {
    return ps.program[index + i];
}

char Position::get() {
    char res = cur();
    ++(*this);
    return res;
}

bool Position::operator<(const Position &p) const {
    return start < p.start;
}

bool Position::operator==(const Position &p) const {
    return start == p.start;
}

bool Position::end_of_program() const {
    return start == ps.end;
}

int Position::is_at_newline() {
    if (cur() == '\n') return cur_type::NLINE;
    if (index + 1 < ps.length &&
        cur() == '\r' && peek() == '\n') {
        return cur_type::WNLINE;
    }
    return cur_type::CHAR;
}

Position &Position::operator++() {
    if (!end_of_program()) {
        int at_newline = is_at_newline();
        if (at_newline == CHAR) {
            start.inc_pos();
            ++index;
        } else {
            start.inc_line();
            index += at_newline;
        }
    }

    return *this;
}


Token::Token(const Token &t) : start(t.start), end(t.end), raw(t.raw), _ident(t._ident), _tag(t._tag) {}

Token::Token(const Position& s, const Position& e, Tag t, std::string r)
        : start(s), end(e), _tag(t), raw(std::move(r)) {
    _ident = t_info[_tag].name;
}

Token& Token::operator=(const Token &t) {
    if (&t != this) {
        start = t.start;
        end = t.end;
        raw = t.raw;
        _ident = t._ident;
        _tag = t._tag;
    }
    return *this;
}

void Token::convert() {
    Tag alt = t_info[_tag].alternative_tag;
    if (alt) {
        _tag = alt;
        _ident = t_info[_tag].name;
    }
}

void Token::unary() {
    if (t_info[_tag].is_binary) convert();
}

void Token::binary() {
    if (!t_info[_tag].is_binary) convert();
}

bool Token::operator<(const Token &t) const {
    return start < t.start;
}

bool Token::operator==(const Token &t) const {
    return start == t.start;
}


std::string to_string(const Coordinate& c) {
    return "(" + std::to_string(c.line) + ", " + std::to_string(c.pos) + ")";
}

std::string to_string(const ProgramString& ps) {
    return "ProgramString " + to_string(ps.begin) + "-" + to_string(ps.end) +
           " (" + std::to_string(ps.length) + ")\n" + ps.program;
}

std::string to_string(const Position& p) {
    return "{" + to_string(p.start) + ", " + std::to_string(p.index) + "}";
}

std::string to_string(const Token& l) {
    std::string res = "<" + to_string(l.start) + "-" + to_string(l.end) +
                      ": " + ((l.raw.empty()) ? "" : (l.raw + "; ")) + l._ident + ">";
    return res;
}
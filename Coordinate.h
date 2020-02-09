#pragma once

#include <string>

typedef struct Coordinate {
    size_t line, pos;   //строка, смещение
    Coordinate(size_t l = 1, size_t p = 1) : line(l), pos(p) {}

    Coordinate &inc_line() {
        line++;
        pos = 1;
        return *this;
    }

    Coordinate &inc_pos() {
        pos++;
        return *this;
    }

    Coordinate(const Coordinate &c) : line(c.line), pos(c.pos) {}

    Coordinate &operator=(const Coordinate &c) {
        if (&c != this) {
            line = c.line;
            pos = c.pos;
        }
        return *this;
    }

    bool operator==(const Coordinate &other) const {
        return (line == other.line) && (pos == other.pos);
    }

    bool operator<(const Coordinate &other) const {
        return (line < other.line) || ((line == other.line) && (pos < other.pos));
    }

    friend std::string to_string(Coordinate c);
} Coordinate;

typedef struct ProgramString {
    std::string program = "";
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

    //Position() {}
    Position(const Position &p) : start(p.start), index(p.index) {}

    Position(Coordinate x = Coordinate(), size_t i = 0) : start(x), index(i) {

        if (ps.end < start) {
            std::cout << "Position:: ps.end < start\n";
            throw std::exception();
        }
    }

    Position &operator=(const Position &p) {
        if (&p != this) {
            start = p.start;
            index = p.index;
        }
        return *this;
    }

    int is_at_newline();

    Position &operator++();

    Position operator++(int) {
        Position tmp(*this);
        operator++();
        return tmp;
    }

    char operator[](int i) const { return ps.program[i]; }

    char &operator[](int i) { return ps.program[i]; }

    char cur() { return ps.program[index]; }

    bool can_peek(int i = 1) { return index + i < ps.length; }

    char peek(int i = 1) { return ps.program[index + i]; }

    char get() {
        char res = cur();
        ++(*this);
        return res;
    }

    bool operator<(const Position &p) const { return start < p.start; }

    bool operator==(const Position &p) const { return start == p.start; }

    friend std::string to_string(Position p);

    bool end_of_program() {
        return start == ps.end;
    }
} Position;

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
    }//else std::cout << "EOF\n";
    return *this;
}


typedef struct Token {
    Position start; //координаты начала и конца
    Position end;
    std::string raw;    //подстрока из файла
    std::string _ident = "";    //строковое представление тега - для принта
    Tag _tag = ERROR;

    Token(const Token &t) : start(t.start), end(t.end), raw(t.raw), _ident(t._ident), _tag(t._tag) {}

    Token(Position s, Position e, Tag t = ERROR, std::string r = "")
            : start(s), end(e), _tag(t), raw(r) {
        _ident = t_info[_tag].name;
    }

    Token &operator=(const Token &t) {
        if (&t != this) {
            start = t.start;
            end = t.end;
            raw = t.raw;
            _ident = t._ident;
            _tag = t._tag;
        }
        return *this;
    }

    //отладочные конструкторы
    /*Token(Tag x) : _tag(x), _ident(t_info[x].name) {}
    Token(double x) : Token(NUMBER) {
    _value = x;
    _ident = std::to_string(x);
    }
    Token(std::string x) : Token(IDENT) { _ident = x; }*/
    void convert() {
        Tag alt = t_info[_tag].alternative_tag;
        if (alt) {
            _tag = alt;
            _ident = t_info[_tag].name;
        }
    }

    void unary() { if (t_info[_tag].is_binary) convert(); }

    void binary() { if (!t_info[_tag].is_binary) convert(); }

    bool operator<(const Token &t) const { return start < t.start; }

    bool operator==(const Token &t) const { return start == t.start; }

    friend std::string to_string(Position p);
} Token;

std::string to_string(Coordinate c) {
    return "(" + std::to_string(c.line) + ", " + std::to_string(c.pos) + ")";
}

std::string to_string(ProgramString ps) {
    return "ProgramString " + to_string(ps.begin) + "-" + to_string(ps.end) +
           " (" + std::to_string(ps.length) + ")\n" + ps.program;
}

std::string to_string(Position p) {
    return "{" + to_string(p.start) + ", " + std::to_string(p.index) + "}";
}

std::string to_string(Token l) {
    std::string res = "<" + to_string(l.start) + "-" + to_string(l.end) +
                      ": " + ((l.raw.empty()) ? "" : (l.raw + "; ")) + l._ident + ">";
    return res;
}

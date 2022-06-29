#include <vector>
#include <string>

#include "Lexer.h"


Lexer::Lexer() = default;;

Lexer::Lexer(const Position& p, const ProgramString&) {
    current = p;
};

Lexer::~Lexer() = default;


std::vector<Token> Lexer::next() {
    std::vector<Token> v;
    if (!current.end_of_program()) {
        Position start = current;
        char c = current.get(); //сохранить текущий символ и перейти на следующий
        std::string tmp;

        if (c == '\n' && isProduct) {
            if (product_iter_tokens.size() == product_tokens.size()) {
                isProduct = false;
                for (int i = product_tokens.size() - 1; i >= 0; i--) {
                    v.push_back(product_iter_tokens[i]);
                    v.emplace_back(start, current, SET);
                    v.push_back(product_iter_tokens[i]);
                    v.emplace_back(start, current, ADD);
                    v.emplace_back(start, current, NUMBER, "1");
                    v.push_back(product_tokens[i]);
                    v.emplace_back(start, current, ENDB, "\\end");
                    v.emplace_back(start, current, ENDB, "\\end");
                }
                return v;
            } else {
                std::cout << "iters != products";  // не должно выполняться
            }
        } else if (c == '\n' && isSum) {
            if (sum_tokens.size() == sum_iter_tokens.size()) {
                isSum = false;
                for (int i = sum_tokens.size() - 1; i >= 0; i--) {
//                    std::cout << "iter = " << i << std::endl;
                    v.push_back(sum_iter_tokens[i]);
                    v.emplace_back(start, current, SET);
                    v.push_back(sum_iter_tokens[i]);
                    v.emplace_back(start, current, ADD);
                    v.emplace_back(start, current, NUMBER, "1");
                    v.emplace_back(start, current, ENDB, "\\end");
                    v.push_back(sum_tokens[i]);
                    v.emplace_back(start, current, ENDB, "\\end");
                }
                return v;
            } else {
                std::cout << "iters != sums";  // не должно выполняться
            }
        } else if (isspace(c)) {
            while (isspace(current.cur())) current++;
            v.emplace_back(start, current, SPACE);
            return v;
        } else if (c == '%') { //комментарии игнорируются до следующей строки
            do { current++; } while (!current.is_at_newline());
            v.emplace_back(start, current, SPACE);
            return v;
        } else if (c == '\\') {
            if (!current.end_of_program() && current.cur() == '\\') {
                current++;
                v.emplace_back(start, current, BREAK, "\\\\");
                return v;
            }
            for (tmp = c; isalpha(current.cur());) tmp += current.get();

            Tag tmp_tag = KEYWORD;
            auto res = raw_tag.find(tmp);
            if (res != raw_tag.end()) {
                tmp_tag = res->second;
                std::string attrib;
                if (tmp_tag == PLACEHOLDER) {
                    if (current.cur() == '[') {
                        current.get();
                        isPlaceholder = true;

                        auto tmp_cur = current;
                        while (current.cur() != ']') current.get();
                        current.get();
                        if (!get_attribute(attrib)) throw Error(current.start, "Expected {...}");

                        v.emplace_back(start, current, PLACEHOLDER, tmp);
                        v.emplace_back(start, tmp_cur, DIV);
                        v.emplace_back(start, tmp_cur, LPAREN);
                        current = tmp_cur;
                        return v;
                    } else {
                        if (!get_attribute(attrib)) throw Error(current.start, "Expected {...}");
                        v.emplace_back(start, current, PLACEHOLDER, tmp);
                        return v;
                    }
                }
                if (tmp_tag == BEGIN || tmp_tag == END) {
                    if (!get_attribute(attrib)) throw Error(current.start, "Expected {...}");
                    switch (tmp_tag) {
                        case BEGIN:
                            if (attrib == "{block}") {
                                v.emplace_back(start, current, BEGINB, tmp);
                                return v;
                            } else if (attrib == "{caseblock}") {
                                v.emplace_back(start, current, BEGINC, tmp);
                                return v;
                            } else if (attrib == "{pmatrix}") {
                                v.emplace_back(start, current, BEGINM, tmp);
                                return v;
                            }
                        case END:
                            if (attrib == "{block}") {
                                v.emplace_back(start, current, ENDB, tmp);
                                return v;
                            } else if (attrib == "{caseblock}") {
                                v.emplace_back(start, current, ENDC, tmp);
                                return v;
                            } else if (attrib == "{pmatrix}") {
                                v.emplace_back(start, current, ENDM, tmp);
                                return v;
                            }
                        default:
                            break;
                    }
                } else if (tmp_tag == SUM) {
                    current.get(); // прочитали _

                    sumName = "sum" + std::to_string(random());

                    v.emplace_back(start, current, BEGINB, "\\begin");

                    v.emplace_back(start, current, IDENT, sumName);
                    v.emplace_back(start, current, SET);
                    v.emplace_back(start, current, NUMBER, "0");

                    std::string lower_bound;
                    if (!get_attribute(lower_bound)) throw Error(current.start, "Expected {...}");
                    std::vector<Token> lower = parse_sum_lower_bound(lower_bound);

                    current.get(); // прочитали ^

                    std::string upper_bound;
                    if (!get_attribute(upper_bound)) throw Error(current.start, "Expected {...}");
                    std::vector<Token> upper = parse_sum_upper_bound(upper_bound);

                    v.insert(v.end(), lower.begin(), lower.end());
                    v.emplace_back(start, current, WHILE, "\\while");
                    v.emplace_back(start, current, LBRACE);
                    v.push_back(lower[0]);
                    v.emplace_back(start, current, LEQ);
                    v.push_back(upper[0]);
                    v.emplace_back(start, current, RBRACE);
                    v.emplace_back(start, current, BEGINB, "\\begin");

                    v.emplace_back(start, current, IDENT, sumName);
                    v.emplace_back(start, current, SET);
                    v.emplace_back(start, current, IDENT, sumName);
                    v.emplace_back(start, current, ADD);
                    sum_tokens.emplace_back(start, current, IDENT, sumName);
                    sum_iter_tokens.push_back(lower[0]);

                    isSum = true;

                    return v;
                } else if (tmp_tag == PRODUCT) {
                    isProduct = true;

                    productName = "product" + std::to_string(random());

                    current.get(); // прочитали _


                    v.emplace_back(start, current, BEGINB, "\\begin");
                    v.emplace_back(start, current, IDENT, productName);
                    v.emplace_back(start, current, SET);
                    v.emplace_back(start, current, NUMBER, "1");

                    std::string lower_bound;
                    if (!get_attribute(lower_bound)) throw Error(current.start, "Expected {...}");
                    std::vector<Token> lower = parse_sum_lower_bound(lower_bound);

                    current.get(); // read ^

                    std::string upper_bound;
                    if (!get_attribute(upper_bound)) throw Error(current.start, "Expected {...}");
                    std::vector<Token> upper = parse_sum_upper_bound(upper_bound);

                    v.insert(v.end(), lower.begin(), lower.end());
                    v.emplace_back(start, current, PRODUCT, "\\product");
                    //cond
                    v.emplace_back(start, current, LBRACE);
                    v.push_back(lower[0]);
                    v.emplace_back(start, current, LEQ);
                    v.push_back(upper[0]);
                    v.emplace_back(start, current, RBRACE);

                    v.emplace_back(start, current, BEGINB, "\\begin");
                    v.emplace_back(start, current, IDENT, productName);
                    v.emplace_back(start, current, SET);
                    v.emplace_back(start, current, IDENT, productName);
                    v.emplace_back(start, current, MUL);

                    product_tokens.emplace_back(start, current, IDENT, productName);
                    product_iter_tokens.push_back(lower[0]);

                    return v;
                } else if (tmp_tag == FLOOR) {
                    isFloor = true;

                    current.get(); //read *
                    if (current.get() == '{') {
                        v.emplace_back(start, current, KEYWORD, "\\floor");
                        v.emplace_back(start, current, LPAREN);
                    } else throw Error(current.start, "Expected {...}");

                    return v;
                } else if (tmp_tag == CEIL) {
                    isCeil = true;

                    current.get(); //read *
                    if (current.get() == '{') {
                        v.emplace_back(start, current, KEYWORD, "\\ceil");
                        v.emplace_back(start, current, LPAREN);
                    } else throw Error(current.start, "Expected {...}");

                    return v;
                }
            }
            v.emplace_back(start, current, tmp_tag, tmp);
            return v;
        } else if (isalpha(c)) {
            for (tmp = c; isalpha(current.cur()) || isdigit(current.cur());) tmp += current.get();
            auto res = dim_tag.find(tmp);

            if (res != dim_tag.end()) {
                v.emplace_back(start, current, DIMENSION, tmp);
                return v;
            } else if (current.can_peek() && current.cur() == '_' &&
                       current.peek() == '\\') {   //это не может быть индекс, потому что после '_' идет '\'
                tmp += current.get();   //прочитать '_'
                std::string kw;
                kw += current.get();
                while (isalpha(current.cur())) { kw += current.get(); } //прочитать '\text'
                if (kw != "\\text") throw Error(current.start, "Expected \\text{...}");
                if (!get_attribute(kw)) throw Error(current.start, "Expected {...}");
                tmp += kw;
            }
            v.emplace_back(start, current, IDENT, tmp);
            return v;
        } else if (isdigit(c)) {
            tmp = c;
            if (c != '0') while (isdigit(current.cur())) tmp += current.get();
            if (!current.end_of_program() && current.cur() == '.') {
                tmp += current.get();
                while (isdigit(current.cur())) tmp += current.get();
            }
            v.emplace_back(start, current, NUMBER, tmp);
            return v;
        } else {
            switch (c) {
                case '+':
                    v.emplace_back(start, current, ADD);
                    return v;
                case '-':
                    v.emplace_back(start, current, SUB);
                    return v;
                case '*':
                    v.emplace_back(start, current, MUL);
                    return v;
                case '/':
                    v.emplace_back(start, current, DIV);
                    return v;
                case '^':
                    v.emplace_back(start, current, POW);
                    return v;
                case '(':
                    v.emplace_back(start, current, LPAREN);
                    return v;
                case ')':
                    v.emplace_back(start, current, RPAREN);
                    return v;
                case ',':
                    v.emplace_back(start, current, COMMA);
                    return v;
                case '{':
                    if (!isPlaceholder) {
                        v.emplace_back(start, current, LBRACE);
                    } else {
                        v.emplace_back(current, current, SKIP);
                    }
                    return v;
                case '}':
                    if (!isPlaceholder && !isFloor && !isCeil) {
                        v.emplace_back(start, current, RBRACE);
                    } else {
                        if (isPlaceholder) {
                            isPlaceholder = false;
                            v.emplace_back(current, current, SKIP);
                        } else if (isFloor) {
                            isFloor = false;
                            v.emplace_back(start, current, RPAREN);
                        } else {
                            isCeil = false;
                            v.emplace_back(start, current, RPAREN);
                        }
                    }
                    return v;
                case '[':
                    v.emplace_back(start, current, LBRACKET);
                    return v;
                case ']':
                    if (!isPlaceholder) {
                        v.emplace_back(start, current, RBRACKET);
                    } else {
                        v.emplace_back(start, current, RPAREN);
                    }
                    return v;
                case '_':
                    v.emplace_back(start, current, INDEX);
                    return v;
                case '<':
                    v.emplace_back(start, current, LT);
                    return v;
                case '>':
                    v.emplace_back(start, current, GT);
                    return v;
                case '=':
                    v.emplace_back(start, current, EQ);
                    return v;
                case '&':
                    v.emplace_back(start, current, AMP);
                    return v;
                case ':':
                    if (!current.end_of_program() && current.cur() == '=') {
                        v.emplace_back(start, ++current, SET);
                        return v;
                    }
                default:
                    v.emplace_back(start, current);
                    return v;
            }
        }
    }
    v.emplace_back(current, current, NONE);
    return v;
}

std::vector<Token> Lexer::program_to_tokens(const ProgramString& ps) {
    std::vector<Token> res;
    Position::ps = ps;
    current = Position(ps.begin, ps.begin.pos - 1);
    Tag t;
    bool skip = false;
    do {
        std::vector<Token> x = next();

        t = x[0]._tag;
        if (t == BEGIN) {
            skip = true;
        }
        if (!skip && t != SPACE && t != SKIP) {
            if (t == ERROR) {
                throw Error(x[0].start.start, "Unexpected symbol");
            }
            res.insert(res.end(), x.begin(), x.end());
        }
        if (t == END) {
            skip = false;
        }
    } while (t != NONE);
    return res;
}

bool Lexer::get_attribute(std::string &s) {
    while (isspace(current.cur())) ++current;   //нужно ли скипать пробелы?
    if (current.cur() != '{') return false;
    int lb = 0, rb = 0;
    do {
        char c = current.get();
        if (c == '\\') current.get();
        else if (c == '{') lb++;
        else if (c == '}') rb++;
        s += c;
    } while (lb != rb && !current.end_of_program());
    return lb == rb;
}

std::vector<Token> Lexer::parse_sum_lower_bound(std::string s) {
    Position start = current;
    std::vector<Token> v;
    int i = 1;
    std::string ident;
    std::string bound;

    bool ident_not_found = true;
    bool bound_not_found = true;

    while (i < s.length() && (ident_not_found || bound_not_found)) {
        if (isalpha(s[i])) {
            while (isalpha(s[i])) {
                ident += s[i];
                i++;
            }
            ident_not_found = false;
        } else if (isdigit(s[i])) {
            bound = s[i];
            if (s[i] != '0') {
                i++;
                while (isdigit(s[i])) {
                    bound += s[i];
                    i++;
                }
            }
            if (s[i] == '.') {
                bound += s[i];
                i++;
                while (isdigit(s[i])) {
                    bound += s[i];
                    i++;
                }
            }
            bound_not_found = false;
        }
        i++;
    }
    v.emplace_back(start, current, IDENT, ident);
    v.emplace_back(start, current, SET);
    v.emplace_back(start, current, NUMBER, bound);

    return v;
}

std::vector<Token> Lexer::parse_sum_upper_bound(std::string s) {
    Position start = current;
    std::vector<Token> v;
    int i = 1;
    std::string bound;

    bool bound_not_found = true;

    while (i < s.length() && bound_not_found) {
        if (isdigit(s[i])) {
            bound = s[i];
            if (s[i] != '0') {
                i++;
                while (isdigit(s[i])) {
                    bound += s[i];
                    i++;
                }
            }
            if (s[i] == '.') {
                bound += s[i];
                i++;
                while (isdigit(s[i])) {
                    bound += s[i];
                    i++;
                }
            }
            bound_not_found = false;
            v.emplace_back(start, current, NUMBER, bound);
        } else if (isalpha(s[i])) {
            while (isalpha(s[i])) {
                bound += s[i];
                i++;
            }
            bound_not_found = false;
            v.emplace_back(start, current, IDENT, bound);
        }
        i++;
    }
    return v;
}
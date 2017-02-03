#pragma once

#include <vector>

class Lexer {
private:
	Position current;
	bool get_attribute(std::string&);
	Token next();
public:
	Lexer() {}
	~Lexer() {}
	std::vector<Token> program_to_tokens(ProgramString);
};

Token Lexer::next() {
	if (!current.end_of_program()) {
		Position start = current;
		char c = current.get(); //сохранить текущий символ и перейти на следующий
		std::string tmp = "";
		if (isspace(c)) {
			while (isspace(current.cur())) current++;
			return Token(start, current, SPACE);
		}
		else if (c == '%') { //комментарии игнорируются до следующей строки
			do { current++; } while (!current.is_at_newline());
			return Token(start, current, SPACE);
		}
		else if (c == '\\') {
			if (!current.end_of_program() && current.cur() == '\\') {
				current++;
				return Token(start, current, BREAK, "\\\\");
			}
			for (tmp = c; isalpha(current.cur());) tmp += current.get();
			Tag tmp_tag = KEYWORD;
			auto res = raw_tag.find(tmp);
			if (res != raw_tag.end()) {
				tmp_tag = res->second;
				std::string attrib = "";
				if (tmp_tag == PLACEHOLDER || tmp_tag == BEGIN || tmp_tag == END) {
					if (!get_attribute(attrib)) throw Error(current.start, "Expected {...}");
					switch (tmp_tag) {
					case BEGIN:
						if (attrib == "{block}") return Token(start, current, BEGINB, tmp);
						else if (attrib == "{caseblock}") return Token(start, current, BEGINC, tmp);
						else if (attrib == "{pmatrix}") return Token(start, current, BEGINM, tmp);
					case END:
						if (attrib == "{block}") return Token(start, current, ENDB, tmp);
						else if (attrib == "{caseblock}") return Token(start, current, ENDC, tmp);
						else if (attrib == "{pmatrix}") return Token(start, current, ENDM, tmp);
					default:
						break;
					}
				}
			}
			return Token(start, current, tmp_tag, tmp);
		}
		else if (isalpha(c)) {
			for (tmp = c; isalpha(current.cur()) || isdigit(current.cur());) tmp += current.get();
			if (current.can_peek() && current.cur() == '_' && current.peek() == '\\') {   //это не может быть индекс, потому что после '_' идет '\'
				tmp += current.get();   //прочитать '_'
				std::string kw = ""; kw += current.get();
				while (isalpha(current.cur())) { kw += current.get(); } //прочитать '\text'
				if (kw != "\\text") throw Error(current.start, "Expected \\text{...}");
				if (!get_attribute(kw)) throw Error(current.start, "Expected {...}");
				tmp += kw;
			}
			return Token(start, current, IDENT, tmp);
		}
		else if (isdigit(c)) {
			tmp = c;
			if (c != '0') while (isdigit(current.cur())) tmp += current.get();
			if (!current.end_of_program() && current.cur() == '.') {
				tmp += current.get();
				while (isdigit(current.cur())) tmp += current.get();
			}
			return Token(start, current, NUMBER, tmp);
		}
		else {
			switch (c) {
			case '+':
				return Token(start, current, ADD);
			case '-':
				return Token(start, current, SUB);
			case '*':
				return Token(start, current, MUL);
			case '/':
				return Token(start, current, DIV);
			case '^':
				return Token(start, current, POW);
			case '(':
				return Token(start, current, LPAREN);
			case ')':
				return Token(start, current, RPAREN);
			case ',':
				return Token(start, current, COMMA);
			case '{':
				return Token(start, current, LBRACE);
			case '}':
				return Token(start, current, RBRACE);
			case '[':
				return Token(start, current, LBRACKET);
			case ']':
				return Token(start, current, RBRACKET);
			case '_':
				return Token(start, current, INDEX);
			case '<':
				return Token(start, current, LT);
			case '>':
				return Token(start, current, GT);
			case '=':
				return Token(start, current, EQ);
			case '&':
				return Token(start, current, AMP);
			case ':':
				if (!current.end_of_program() && current.cur() == '=') {
					return Token(start, ++current, SET);
				}
			default:
				return Token(start, current);
				//throw Error(current.start, "Unexpected symbol");
			}
		}
	}
	return Token(current, current, NONE);
}

std::vector<Token> Lexer::program_to_tokens(ProgramString ps) {
	std::vector<Token> res;
	Position::ps = ps;
	current = Position(ps.begin, ps.begin.pos - 1);
	Tag t;
	bool skip = false;
	do {
		Token x = next();

		t = x._tag;
		if (t == BEGIN) {
			skip = true;
		}
		if (!skip && t != SPACE) {
			if (t == ERROR) {
				throw Error(x.start.start, "Unexpected symbol");
			}
			//std::cout << to_string(x) << std::endl;
			res.push_back(x);
		}
		if (t == END) {
			skip = false;
		}
	} while (t != NONE);
	return res;
}

bool Lexer::get_attribute(std::string& s) {
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

#pragma once

class Value;

class Node;

typedef struct Parser {
	std::vector<Token> program;
	int i = 0;

	Token *next() { return &program[++i]; }

	Token *cur() { return &program[i]; }

	Token *get() { return &program[i++]; }

	bool skip(Tag);

	void init(std::vector<Token> &);

	Node *unexpr(Token *);

	Node *binexpr(Token *, Node *);

	Node *expression(int pr);

	std::vector<Node *> block(Tag stop = NONE);

	std::vector<Node *> line();

	std::vector<Node *> cases();

	std::vector<Node *> list(Tag close);

	std::vector<Node *> matrix();

	Node * arg(Tag open);

	void wait(Tag stop);
} Parser;

bool Parser::skip(Tag x) {
	if (x != 0 && (cur()->_tag != x)) {
		return false;
		//throw Error(cur()->start.start, "Unexpected symbol");
	}
	else ++i;
	return true;
}

void Parser::init(std::vector<Token> &ts) {
	program = ts;
	i = 0;
}

typedef std::map<std::string, Value> name_table;
//typedef std::map<Coordinate, std::pair<std::pair<size_t, size_t>, Value>> replacement_map;

struct Replacement;

typedef std::map<Coordinate, Replacement> replacement_map;

class Node {
	friend struct Parser;

	Coordinate _coord;
	Tag _tag = ERROR;
	std::string _label = "";
	int _priority = 0;
public:
	static name_table global;
	static replacement_map reps;
	Node *left = nullptr;
	Node *right = nullptr;
	Node *cond = nullptr;
	std::vector<Node *> fields;

	Node() {}

	Node(const Node &n) : _coord(n._coord), _tag(n._tag), _label(n._label), _priority(n._priority) {
		if (n.left) left = new Node(*n.left);
		if (n.right) right = new Node(*n.right);
		if (n.cond) cond = new Node(*n.cond);
		for (auto it = n.fields.begin(); it != n.fields.end(); ++it) {
			fields.push_back(new Node(**it));
		}
	}

	Node(Token *t);

	static void save_rep(Coordinate, Tag, size_t, size_t);

	~Node() {
		if (left) delete left;
		if (right) delete right;
		if (cond) delete cond;
		for (auto it = fields.begin(); it != fields.end(); ++it) {
			delete *it;
		}
	}

	void print(std::string pref) const {
		std::string img = t_info[_tag].name + ((_label.empty()) ? "" : "(" + _label + ")");
		printf("%s Node: %s, %d\n", pref.c_str(), img.c_str(), _priority);
		if (left) left->print(pref + "l");
		if (right) right->print(pref + "r");
		if (cond) cond->print(pref + "c");
		size_t n = fields.size();
		for (size_t i = 0; i < n; i++) {
			fields[i]->print(pref + "[" + std::to_string(i) + "]");
		}
	}

	void set_tag(Tag t) {
		_tag = t;
		_label = t_info[_tag].name;
		_priority = t_info[_tag].priority;
	}

	Value exec(name_table *nt);

	static void copy_defs(name_table &local, name_table *ptr);

	static Value &lookup(std::string name, name_table *ptr, Coordinate);

	static void def(std::string name, Value, name_table *ptr);
};

Node *Parser::expression(int pr = 0) {  //выражение
	Token *ptr = get();
	Node *lhs = Parser::unexpr(ptr);
	while (pr < t_info[cur()->_tag].priority) {
		ptr = get();
		lhs = Parser::binexpr(ptr, lhs);
	}
	return lhs;
}

std::vector<Node *> Parser::block(Tag stop) {   //блок
	std::vector<Node *> block;
	while (cur()->_tag != stop) {
		if (cur()->_tag == BREAK) {
			get();
			continue;
		}
		block.push_back(expression(0));
	}
	return block;
}

std::vector<Node *> Parser::line() {
	std::vector<Node *> res;
	Tag ctag = cur()->_tag;
	if (ctag == AMP || ctag == BREAK || ctag == ENDM) {
		throw Error(cur()->start.start, "Bad matrix");
	}
	res.push_back(expression(0));
	while (cur()->_tag == AMP) {
		get();
		res.push_back(expression(0));
	}
	return res;
}

std::vector<Node *> Parser::matrix() {
	std::vector<Node *> res;
	Node *row = new Node();
	row->set_tag(LIST);
	row->_coord = cur()->start.start;
	row->fields = line();
	res.push_back(row);
	size_t N = row->fields.size();
	while (cur()->_tag == BREAK) {
		get();
		row = new Node();
		row->set_tag(LIST);
		row->_coord = cur()->start.start;
		row->fields = line();
		res.push_back(row);
		if (N != row->fields.size()) {
			for (auto it = row->fields.begin(); it != row->fields.end(); ++it) {
				delete *it;
				throw Error(row->_coord, "Matrix is not rectangular");
			}
		}
	}
	return res;
}

std::vector<Node *> Parser::cases() {
	std::vector<Node *> res;
	do {
		Node *alt = new Node();
		alt->set_tag(ALT);
		alt->_coord = cur()->start.start;
		alt->right = expression(0);

		Tag t = get()->_tag;    // тег должен быть WHEN или OTHERWISE
		if (t == WHEN) {
			alt->cond = expression(0);
		}
		else if (t != OTHERWISE) {
			delete alt;
			for (auto it = res.begin(); it != res.end(); ++it) {
				delete *it;
			}
			throw Error(cur()->start.start, "Unexpected symbol - expected \\when or \\otherwise");
		}
		res.push_back(alt);

		if (cur()->_tag == BREAK) {
			get();
			continue;
		}
	} while (cur()->_tag != ENDC);

	return res;
}

std::vector<Node *> Parser::list(Tag close) {  //список аргументов
	std::vector<Node *> res;
	Tag next = Parser::next()->_tag;   //скипнуть (, перейти на следующий токен
	if (next == close) {            //пустой список, скипнуть )
		get();
	}
	else {
		do {
			res.push_back(expression(0));      //распарсить Expr в left
			next = get()->_tag;             //если запятая, то продолжить цикл
		} while (next == COMMA);
		if (next != close) {               //если выход не на ), то это ошибка
			for (auto it = res.begin(); it != res.end(); ++it) {
				delete *it;
			}
			throw Error(cur()->start.start, "List not closed");
		}
	}
	return res;
}

Node *Parser::binexpr(Token *rhs, Node *lhs) {
	rhs->binary();
	Node *res = new Node(rhs);
	Tag close_tag = t_info[res->_tag].close_tag;

	res->left = lhs;
	int pr = res->_priority;
	if (t_info[res->_tag].is_inverted) --pr;
	res->right = expression(pr);

	if (close_tag) {    //это вообще когда-нибудь срабатывает?
		if (!skip(close_tag)) {
			delete res;
			throw Error(cur()->start.start, "Unexpected symbol");
		}
	}

	return res;
}

Node *Parser::unexpr(Token *t) {
	t->unary();
	//if (t->_tag == NOT) std::cout << "AAAAAAAAAAAAAAAAAAAAAA\n";
	Node *res = new Node(t);
	Tag close_tag = t_info[res->_tag].close_tag;
	// если это выражение в скобках
	if (close_tag) {
		if (res->_tag == BEGINB) {
			res->fields = block(close_tag);
		}
		else if (res->_tag == BEGINC) {
			res->fields = cases();
		}
		else if (res->_tag == BEGINM) {
			res->fields = matrix();
		}
		else {    //выражение в простых скобках
			delete res;
			res = expression(0);
		}
		if (!skip(close_tag)) {
			delete res;
			throw Error(cur()->start.start, "Unexpected symbol - expected close_tag");
		}
	}
	else if (res->_tag == IDENT) {
		if (cur()->_tag == INDEX) {  //обращение по индексу
			get();
			if (cur()->_tag == LBRACE) { //составной индекс: x_{1+2+3}, y_{q,w}
				res->fields = list(RBRACE);
				if (res->fields.size() < 1 || res->fields.size() > 2) {
					throw Error(res->_coord, "Bad index");
				}
			}
			else {  //простой индекс: x_1, y_z
				res->fields.push_back(unexpr(get()));
			}
		}
		if (cur()->_tag == LPAREN) { //если есть список аргументов, то это функция
			res->_tag = FUNC;
			res->fields = list(RPAREN);
		}
	}
	else if (res->_tag == KEYWORD) {
		if (cur()->_tag == LPAREN) {
			res->fields = list(RPAREN); //тег ключевого слова не надо менять на тег функции
		}
	}
	//\newcommand{\range}[3][]
	else if (res->_tag == RANGE) {
		if (cur()->_tag == LBRACKET) {	//опциональный параметр - шаг
			res->cond = expression(0);
		}
		res->left = arg(LBRACE);
		res->right = arg(LBRACE);
	}
	else if (res->_tag == TRANSP) {
		res->left = arg(LBRACE);
	}
	else if (res->_tag == FRAC) {
		res->left = arg(LBRACE);
		res->right = arg(LBRACE);
	}
	else if (res->_tag == IF) {    //If ::= \ifexpr{ Expr } Operator (\otherwise Operator)?
		res->cond = arg(LBRACE);
		if (cur()->_tag == BREAK) {
			get();
		}
		res->right = expression(0);
		if (cur()->_tag == BREAK) {
			get();
		}
		if (cur()->_tag == OTHERWISE) {
			get();
			if (cur()->_tag == BREAK) {
				get();
			}
			res->left = expression(0);
		}
	}
	else if (res->_tag == WHILE) {
		res->cond = expression(0);
		if (cur()->_tag == BREAK) {
			get();
		}
		res->right = expression(0);
	}
	//\newcommand{\graphic}[3]
	else if (res->_tag == GRAPHIC) {
		delete res;
		Node *tmp = arg(LBRACE);	//имя функции
		if (tmp->_tag != IDENT) {
			throw Error(tmp->_coord, "Expected identificator");
		}
		res = tmp;
		res->_tag = GRAPHIC;
		if (cur()->_tag != LBRACE) {
			delete res;
			throw Error(cur()->start.start, "Expected argument");
		}
		res->fields = list(RBRACE);	//поля
		size_t a = cur()->start.index;
		Parser::wait(RBRACE);				//точки графика парсить не нужно
		size_t b = cur()->start.index;
		Node::save_rep(res->_coord, GRAPHIC, a, b);
	}
	else if (t_info[res->_tag].is_operator) {
		res->right = expression(res->_priority);
	}
	return res;
}

//читает аргумент в скобках
Node* Parser::arg(Tag open) {
	if (cur()->_tag != open) {
		throw Error(cur()->start.start, "Expected argument");
	}
	return expression(666);
}

void Parser::wait(Tag stop) {
	while (get()->_tag != stop) {}
}

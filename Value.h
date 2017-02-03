#pragma once

#include <cassert>
#include "Node.h"

typedef struct Func {
	std::vector<std::string> argv;
	name_table local;
	Node *body;

	Func(const Func &f) : argv(f.argv) {
		local = f.local;
		body = new Node(*f.body);
	}

	Func(std::vector<std::string> as, name_table nt, Node *b) : argv(as), local(nt), body(b) {}
} Func;

typedef std::vector<std::vector<Value>> Matrix;

class Value {
public:
	typedef enum Type {
		DOUBLE, MATRIX, FUNCTION
	} Type;

	static std::string type_string(Type t) {
		switch (t) {
		case DOUBLE: return "DOUBLE";
		case MATRIX: return "MATRIX";
		case FUNCTION: return "FUNCTION";
		default: assert(false);
		}
	}

	class BadType : public std::exception {
	private:
		std::string msg;
	public:
		BadType(Type actual, Type expected) {
			msg = "BadType exception: expected " +
				Value::type_string(expected) + " got " + Value::type_string(actual);
		}

		const char* what() { return msg.c_str(); }
	};

private:

	Type _type;
	union {
		double _double_data;
		std::vector<std::vector<Value>> *_matrix_data;
		Func *_function_data;
	};

public:

	Value() : _type(DOUBLE) {
		_double_data = 0.0;
	}

	Value(double d) : _type(DOUBLE) {
		_double_data = d;
	}

	Value(Matrix m) : _type(MATRIX) {
		_matrix_data = new Matrix(m.size());
		for (size_t i = 0; i < m.size(); ++i) {
			for (size_t j = 0; j < m[i].size(); ++j) {
				(*_matrix_data)[i].push_back(Value(m[i][j]));
			}
		}
	}

	Value(Func *f) : _type(FUNCTION) {
		_function_data = new Func(*f);
	}

	// Внимание! Правильно написать конструктор копирования, оператор присваивания и деструктор.

	Value(const Value &other) : _type(other._type) {
		if (_type == DOUBLE) {
			_double_data = other._double_data;
		}
		else if (_type == MATRIX) {
			_matrix_data = new Matrix(other._matrix_data->size());
			for (size_t i = 0; i < _matrix_data->size(); ++i) {
				for (size_t j = 0; j < (*other._matrix_data)[i].size(); ++j) {
					(*_matrix_data)[i].push_back(Value((*other._matrix_data)[i][j]));
				}
			}
		}
		else {
			_function_data = new Func(*other._function_data);
		}
	}

	Value &operator=(const Value &other) {
		if (&other != this) {
			if (_type == DOUBLE) {
				_double_data = 0.0;
			}
			else if (_type == MATRIX) {
				delete _matrix_data;
			}
			else {// if (_type == FUNCTION) {
				delete _function_data;
			}
			_type = other._type;
			if (_type == DOUBLE) {
				_double_data = other._double_data;
			}
			else if (_type == MATRIX) {
				_matrix_data = new Matrix(other._matrix_data->size());
				for (size_t i = 0; i < _matrix_data->size(); ++i) {
					for (size_t j = 0; j < (*other._matrix_data)[i].size(); ++j) {
						(*_matrix_data)[i].push_back(Value((*other._matrix_data)[i][j]));
					}
				}
			}
			else {
				_function_data = new Func(*other._function_data);
			}
		}
		return *this;
	}

	~Value() {
		if (_type == MATRIX) delete _matrix_data;
		if (_type == FUNCTION) delete _function_data;
	}

	friend std::string to_plot(const Value &matr) {
		if (matr._type == MATRIX) {
			std::string res;
			Matrix *m = &matr.get_matrix();
			for (auto it = m->begin(); it != m->end(); ++it) {
				res += "(" + std::to_string((*it)[0].get_double()) + ","
					+ std::to_string((*it)[1].get_double()) + ")\n";
			}
			return res;
		}
		assert(false);
	}

	friend std::string to_string(const Value &val) {
		if (val._type == DOUBLE) {
			return std::to_string(val._double_data);
		}
		if (val._type == MATRIX) {
			std::string res = "\\begin{pmatrix}\n";
			for (auto it = (*val._matrix_data).begin();;) {
				auto jt = (*it).begin();
				res += to_string(*jt);
				++jt;
				for (; jt != (*it).end(); ++jt) {
					res += " & ";
					res += to_string(*jt);
				}
				++it;
				if (it != (*val._matrix_data).end()) {
					res += "\\\\\n";
				}
				else break;
			}
			res += "\\end{pmatrix}";
			return res;
		}
		if (val._type == FUNCTION) {
			return "function"; //или должно быть имя?
		}
		assert(false);
	}

	// Функции ниже в зависимости от типа возвращают значение или бросают исключение

	double get_double() const {
		if (_type != DOUBLE) {
			throw BadType(_type, DOUBLE);
		}
		return _double_data;
	}

	Matrix &get_matrix() const {
		if (_type != MATRIX) {
			throw BadType(_type, MATRIX);
		}
		return *_matrix_data;
	}

	Func *get_function() const {
		if (_type != FUNCTION) {
			throw BadType(_type, FUNCTION);
		}
		return _function_data;
	}

	static Value plus(const Value &left, const Value &right, Coordinate pos) {
		if (left._type == DOUBLE) { //если right - не DOUBLE, сработает исключение
			return Value(left.get_double() + right.get_double());
		}
		else if (left._type == MATRIX) {
			Matrix *l = &left.get_matrix();
			Matrix *r = &right.get_matrix();
			if ((*l).size() == (*r).size() && (*l)[0].size() == (*r)[0].size()) {
				Matrix sum(l->size());
				for (size_t i = 0; i < (*l).size(); ++i) {
					for (size_t j = 0; j < (*l)[0].size(); ++j) {
						sum[i].push_back(plus((*l)[i][j], (*r)[i][j], pos));
					}
				}
				return Value(sum);
			}
			else {
				throw Error(pos, "Dimension mismatch");
			}
		}
		throw Error(pos, "Addition by function");
	}

	static Value usub(const Value &arg, Coordinate pos) {
		if (arg._type == DOUBLE) {
			return Value(-arg.get_double());
		}
		else if (arg._type == MATRIX) {
			Matrix *a = &arg.get_matrix();
			Matrix res(a->size());
			for (size_t i = 0; i < res.size(); ++i) {
				for (size_t j = 0; j < res[i].size(); ++j) {
					res[i].push_back(usub((*a)[i][j], pos));
				}
			}
			return Value(res);
		}
		throw Error(pos, "Substitution by function");
	}

	static Value sub(const Value &left, const Value &right, Coordinate pos) {
		if (left._type == DOUBLE) {
			return Value(left.get_double() - right.get_double());
		}
		else if (left._type == MATRIX) {
			Matrix *l = &left.get_matrix();
			Matrix *r = &right.get_matrix();
			if ((*l).size() == (*r).size() && (*l)[0].size() == (*r)[0].size()) {
				Matrix dif(l->size());
				for (size_t i = 0; i < (*l).size(); ++i) {
					for (size_t j = 0; j < (*l)[0].size(); ++j) {
						dif[i].push_back(sub((*l)[i][j], (*r)[i][j], pos));
					}
				}
				return Value(dif);
			}
			else {
				throw Error(pos, "Dimension mismatch");
			}
		}
		throw Error(pos, "Substitution by function");
	}

	static Value mul(const Value &left, const Value &right, Coordinate pos) {
		if (left._type == DOUBLE) {
			if (right._type == DOUBLE) {
				return Value(left.get_double() * right.get_double());
			}
			else if (right._type == MATRIX) {
				Matrix *r = &right.get_matrix();
				Matrix mult((*r).size());
				for (size_t i = 0; i < (*r).size(); ++i) {
					for (size_t j = 0; j < (*r)[0].size(); ++j) {
						mult[i].push_back(mul(left, (*r)[i][j], pos));
					}
				}
				return Value(mult);
			}
		}
		else if (left._type == MATRIX) {
			if (right._type == DOUBLE) {
				return mul(right, left, pos);
			}
			else if (right._type == MATRIX) {
				Matrix *l = &left.get_matrix();
				Matrix *r = &right.get_matrix();
				size_t l_hor = (*l)[0].size();
				size_t l_vert = (*l).size();
				size_t r_vert = (*r).size();
				size_t r_hor = (*r)[0].size();
				if (l_hor == r_vert) {
					Matrix mult((*l).size());
					for (size_t i = 0; i < l_vert; ++i) {
						for (size_t j = 0; j < r_hor; ++j) {
							Value tmp = mul((*l)[i][0], (*r)[0][j], pos);
							for (size_t k = 1; k < (*r).size(); ++k) {
								tmp = plus(tmp, mul((*l)[i][k], (*r)[k][j], pos), pos);
							}
							mult[i].push_back(tmp);
						}
					}
					return Value(mult);
				}
				//скалярное произведение
				else if (l_vert == 1 && r_vert == 1) {	//строка*строка => строка*столбец
					Value res = Value::mul(*l, Value::transp(*r, pos), pos);	//если длины строк равны, mul выполнится
					return Value(res.get_matrix()[0][0]);
				}
				else if (l_hor == 1 && r_hor == 1) {	//столбец*столбец => строка*столбец
					Value res = Value::mul(Value::transp(*l, pos), *r, pos);
					return Value(res.get_matrix()[0][0]);
				}
				throw Error(pos, "Dimension mismatch");
			}
		}
		throw Error(pos, "Multiplication by function");
	}

	static Value div(const Value &left, const Value &right, Coordinate pos) {
		if (left._type == DOUBLE) {
			if (right._type == DOUBLE) {
				double q = right.get_double();
				if (q == 0.0) {
					throw Error(pos, "Division by zero");
				}
				return Value(left.get_double() / q);
			}
			else if (right._type == MATRIX) {
				throw Error(pos, "Division by matrix");
			}
		}
		else if (left._type == MATRIX) {
			if (right._type == DOUBLE) {
				double q = right.get_double();
				if (q == 0.0) {
					throw Error(pos, "Division by zero");
				}
				return mul(Value(1.0 / q), left, pos);
			}
			else if (right._type == MATRIX) {

				throw Error(pos, "Division by matrix");
			}
		}
		throw Error(pos, "Division by function");
	}

	//static Value call(const Value &arg, std::vector<Value> arguments, Coordinate pos);

	static Value eq(const Value &left, const Value &right, Coordinate pos) {
		if (left._type != right._type) {
			return Value(0.0);  //точно не равны
		}
		if (left._type == DOUBLE) {
			return Value(left.get_double() == right.get_double());
		}
		if (left._type == MATRIX) {
			Matrix *l = &left.get_matrix();
			Matrix *r = &right.get_matrix();
			if (l->size() == r->size() && (*l)[0].size() == (*r)[0].size()) {
				for (size_t i = 0; i < l->size(); ++i) {
					for (size_t j = 0; j < (*l)[0].size(); ++j) {
						Value x = eq((*l)[i][j], (*r)[i][j], pos);
						if (x.get_double() == 0.0) return Value(0.0);
					}
				}
				return Value(1.0);
			}
		}
		//функции не понятно как сравнивать
		return Value(0.0);
	}

	static Value le(const Value &left, const Value &right, Coordinate pos) {
		return Value(left.get_double() <= right.get_double());  //иначе не имеет смысла
	}

	static Value ge(const Value &left, const Value &right, Coordinate pos) {
		return Value(left.get_double() >= right.get_double());
	}

	static Value lt(const Value &left, const Value &right, Coordinate pos) {
		return Value(left.get_double() < right.get_double());
	}

	static Value gt(const Value &left, const Value &right, Coordinate pos) {
		return Value(left.get_double() > right.get_double());
	}

	static Value pow(const Value &left, const Value &right, Coordinate pos) {
		return Value(std::pow(left.get_double(), right.get_double()));
	}

	static Value and(const Value &left, const Value &right, Coordinate pos) {
		return Value(left.get_double() && right.get_double());
	}

	static Value or(const Value &left, const Value &right, Coordinate pos) {
		return Value(left.get_double() || right.get_double());
	}

	//static Value call(const Value & arg, std::vector<Value> arguments, name_table * scope, Coordinate pos);
	static Value call(const Value & arg, std::vector<Value> arguments, Coordinate pos);

	static Value transp(const Value &matr, Coordinate pos) {
		Matrix *m = &matr.get_matrix();
		Matrix mt;
		size_t rows = (*m)[0].size();
		size_t cols = (*m).size();
		mt.resize(rows);
		for (size_t i = 0; i < rows; ++i) {
			mt[i].resize(cols);
			for (size_t j = 0; j < cols; ++j) {
				mt[i][j] = (*m)[j][i];
			}
		}
		return Value(mt);
	}
};

typedef struct Replacement {
	Tag tag;
	size_t begin;
	size_t end;
	Value replacement;
	Replacement() : tag(PLACEHOLDER), begin(0), end(0), replacement(Value(0.0)) {}
	Replacement(Tag t, size_t b, size_t e, Value v = Value(0.0))
		: tag(t), begin(b), end(e), replacement(v) {}
} Replacement;

Node::Node(Token *t) {
	_coord = t->start.start;
	_tag = t->_tag;
	if (_tag == NUMBER || _tag == IDENT || _tag == KEYWORD) {
		_label = t->raw;
	}
	else
		_label = t->_ident;
	_priority = t_info[_tag].priority;

	if (_tag == PLACEHOLDER) {
		Node::save_rep(_coord, PLACEHOLDER, t->start.index + strlen("\\placeholder"), t->end.index);
	}
}

void Node::save_rep(Coordinate c, Tag t, size_t a, size_t b) {
	Node::reps[c] = Replacement(t, a, b);
}

void Node::copy_defs(name_table &local, name_table *ptr) {
	if (ptr) local.insert(ptr->begin(), ptr->end());
	else local.insert(global.begin(), global.end());
}

Value &Node::lookup(std::string name, name_table *ptr, Coordinate pos) {
	if (ptr) {
		auto res = ptr->find(name);
		if (res != ptr->end()) {
			return res->second;
		}
	}
	auto res = global.find(name);
	if (res != global.end()) {
		return res->second;
	}
	throw Error(pos, "Undefined variable reference");
}

Value Value::call(const Value &arg, std::vector<Value> arguments, Coordinate pos) {
	Func *f = arg.get_function();
	size_t sz = f->argv.size();
	for (size_t i = 0; i < sz; ++i) {
		f->local[f->argv[i]] = arguments[i];
	}
	return f->body->exec(&f->local);
}


void Node::def(std::string name, Value val, name_table *ptr) {
	if (ptr) {
		auto res = global.find(name);
		if (res == global.end()) {
			(*ptr)[name] = val;
			return;
		}
	}
	global[name] = val;
}

Value Node::exec(name_table *scope = nullptr) {
	if (_tag == NUMBER) {   //если это NUMBER, то в _label записана строка с числом
		double val = std::stod(this->_label);
		return Value(val);
	}
	else if (_tag == BEGINM) {  //это матрица, нужно собрать из полей Matrix
		Matrix m;   //при построении проверяется, что матрица прямоугольная и как минимум 1 х 1, поэтому здесь проверки не нужны
		for (auto it = fields.begin(); it != fields.end(); ++it) {   //цикл по строкам
			std::vector<Value> v;
			for (auto jt = (*it)->fields.begin(); jt != (*it)->fields.end(); ++jt) { //цикл по элементам строк
				v.push_back((*jt)->exec(scope));
			}
			m.push_back(v);
		}
		return Value(m);
	}
	else if (_tag == IDENT) {   //переменная
		Value x_val = Node::lookup(_label, scope, _coord);
		size_t sz = fields.size();
		if (sz == 0) {  //обычная переменная
			return x_val;
		}
		else {
			Matrix *m = &x_val.get_matrix();
			size_t ver = (*m).size();
			size_t hor = (*m)[0].size();
			int int_i = (int)left->fields[0]->exec(scope).get_double();
			if (int_i < 0) {
				throw new Error(left->_coord, "Negative index");
			}
			size_t i = int_i;
			size_t j = 0;
			if (sz == 1) { //элемент вектора
				if (ver == 1) {
					j = i;
					i = 0;
				}
				else if (hor != 1) {
					throw Error(_coord, "Can't use vector index for matrix");
				}
			}
			else if (sz == 2) { //элемент матрицы
				int int_j = (int)left->fields[1]->exec(scope).get_double();
				if (int_j < 0) {
					throw new Error(left->_coord, "Negative index");
				}
				j = int_j;
			}
			if (i >= ver || j >= hor) {
				throw Error(_coord, "Index is out of range");
			}
			return (*m)[i][j];
		}

	}
	else if (_tag == FUNC) {  //вызов функции
		//область видимости переменных -- функция
		Value f_val = Node::lookup(_label, scope, _coord);
		Func *f = f_val.get_function();
		//загрузка значений имен переменных
		size_t f_s = fields.size();
		if (f_s != f->argv.size()) throw Error(_coord, "Wrong argument number");
		std::vector<Value> args;
		for (size_t i = 0; i < f_s; ++i) {
			args.push_back(fields[i]->exec(scope));
		}
		return Value::call(f_val, args, _coord);
	}
	else if (_tag == UADD || _tag == LPAREN) return right->exec(scope);
	else if (_tag == USUB) return Value::usub(right->exec(scope), _coord);
	else if (_tag == NOT) {
		return Value::eq(right->exec(scope), Value(0.0), _coord);
	}
	else if (_tag == SET) {
		if (left->_tag == IDENT) {
			size_t sz = left->fields.size();
			if (sz == 0) {	//переменная
				Node::def(left->_label, right->exec(scope), scope);
			}
			else {	//матрица
				Value *m_val = &Node::lookup(left->_label, scope, left->_coord);
				Matrix *m = &m_val->get_matrix();
				size_t ver = (*m).size();
				size_t hor = (*m)[0].size();
				int int_i = (int)left->fields[0]->exec(scope).get_double();
				if (int_i < 0) {
					throw new Error(left->_coord, "Negative index");
				}
				size_t i = int_i;
				size_t j = 0;
				if (sz == 1) { //элемент вектора
					if (ver == 1) {
						j = i;
						i = 0;
					}
					else if (hor != 1) {
						throw Error(_coord, "Bad index");
					}
				}
				else if (sz == 2) { //элемент матрицы
					int int_j = (int)left->fields[1]->exec(scope).get_double();
					if (int_j < 0) {
						throw new Error(left->_coord, "Negative index");
					}
					j = int_j;
				}
				else {
					throw Error(_coord, "Bad index");
				}
				if (i >= ver || j >= hor) {
					throw Error(_coord, "Index is out of range");
				}
				(*m)[i][j] = right->exec(scope);
				return Value(0.0);
			}
		}
		else if (left->_tag == FUNC) { //функция
			std::vector<std::string> ns;
			//список аргументов функции
			//при объявлении функции допустимы только IDENT в списке аргументов
			for (auto it = left->fields.begin(); it < left->fields.end(); ++it) {
				if ((*it)->_tag != IDENT) {
					throw Error(_coord, "Can't define function");
				}
				for (auto jt = ns.begin(); jt != ns.end(); ++jt) {
					if (*jt == (*it)->_label) {
						throw Error((*it)->_coord, "Dublicate function argument");
					}
				}
				ns.push_back((*it)->_label);
			}
			//если функция объявляется глобально, ссылаться на Node из дерева нельзя
			//т.к. для каждого блока preproc строится новое, а старое удаляется
			Node *copy_of_right = new Node(*right);
			Func *f = (scope) ? new Func(ns, *scope, copy_of_right) : new Func(ns, global, copy_of_right);
			Value func_v = Value(f);
			Node::def(left->_label, func_v, scope);
		}
		else throw Error(_coord, "Cant't define this");
	}
	else if (_tag == ADD) return Value::plus(left->exec(scope), right->exec(scope), _coord);
	else if (_tag == SUB) return Value::sub(left->exec(scope), right->exec(scope), _coord);
	else if (_tag == MUL) return Value::mul(left->exec(scope), right->exec(scope), _coord);
	else if (_tag == DIV || _tag == FRAC) return Value::div(left->exec(scope), right->exec(scope), _coord);
	else if (_tag == POW) return Value::pow(left->exec(scope), right->exec(scope), _coord);
	else if (_tag == EQ) {
		Value res = left->exec(scope);
		if (right->_tag == PLACEHOLDER) {
			reps[right->_coord].replacement = res;
			return Value(1.0); //равенство выполняется, вернуть 1 - нормально
		}
		return Value::eq(res, right->exec(scope), _coord);
	}
	else if (_tag == NEQ) {
		return Value(!Value::eq(left->exec(scope), right->exec(scope), _coord).get_double());
	}
	else if (_tag == LEQ) return Value::le(left->exec(scope), right->exec(scope), _coord);
	else if (_tag == GEQ) return Value::ge(left->exec(scope), right->exec(scope), _coord);
	else if (_tag == LT) return Value::lt(left->exec(scope), right->exec(scope), _coord);
	else if (_tag == GT) return Value::gt(left->exec(scope), right->exec(scope), _coord);
	else if (_tag == AND) return Value::and(left->exec(scope), right->exec(scope), _coord);
	else if (_tag == OR) return Value::or(left->exec(scope), right->exec(scope), _coord);
	else if (_tag == ROOT) {
		Value res(0.0);
		for (auto it = fields.begin(); it != fields.end(); ++it) {
			res = (*it)->exec(scope);
		}
		return res;
	}
	else if (_tag == BEGINB) {
		Value res(0.0);
		for (auto it = fields.begin(); it != fields.end(); ++it) {
			res = (*it)->exec(scope);
		}
		return res;
	}
	else if (_tag == BEGINC) {
		for (auto it = fields.begin(); it != fields.end(); ++it) {
			if (!(*it)->cond || (*it)->cond->exec(scope).get_double() == 1.0) {
				return (*it)->right->exec(scope);
			}
		}
	}
	else if (_tag == IF) {
		Value c_val = cond->exec(scope);
		if (c_val.get_double()) return right->exec(scope);
		else if (left) return left->exec(scope);
	}
	else if (_tag == WHILE) {
		Value res(0.0);
		int i = 0;
		while (i++ < 10 && cond->exec(scope).get_double() == 1.0) {
			res = right->exec(scope);
		}
		return res;
	}
	else if (_tag == TRANSP) {
		return Value::transp(left->exec(scope), _coord);
	}
	else if (_tag == RANGE) {
		std::vector<Value> row;
		double a = left->exec(scope).get_double();
		double b = right->exec(scope).get_double();
		double d = (cond) ? Value(cond->exec(scope)).get_double() : 0.1;
		for (double x = a; x <= b; x += d) {
			row.push_back(Value(x));
		}
		if (row.empty()) {
			throw Error(_coord, "Empty range");
		}
		Matrix m;
		m.push_back(row);
		return Value(m);
	}
	else if (_tag == GRAPHIC) {
		Value func_v = Node::lookup(_label, scope, _coord);
		Func *func = func_v.get_function();
		size_t sz = func->argv.size();
		std::vector<Value> args(sz);
		size_t ivar = 0;	//номер переменного аргумента
		bool found = false;
		for (size_t i = 0; i < sz; ++i) {
			if (fields[i]->_tag == RANGE) {
				if (!found) {
					ivar = i;
					found = true;
				}
				else {
					throw Error(fields[i]->_coord, "More than one parameter range");
				}
			}
			else {
				args[i] = fields[i]->exec(scope);
			}
		}
		if (!found) {
			throw Error(_coord, "No range parameter");
		}
		Value range_v = fields[ivar]->exec(scope);
		Matrix *range = &range_v.get_matrix();
		Matrix plot;
		for (auto it = (*range)[0].begin(); it != (*range)[0].end(); ++it) {
			args[ivar] = *it;
			double fx = func_v.call(func, args, _coord).get_double();
			std::vector<Value> point = { *it, Value(fx) };
			plot.push_back(point);
		}
		Value graphic(plot);
		Node::reps[_coord].replacement = graphic;
	}
	else if (_tag == KEYWORD) {
		auto res = constants.find(_label);
		if (res != constants.end()) {
			return Value(res->second);
		}
		else {
			auto res = arg_count.find(_label);
			if (res == arg_count.end()) throw Error(_coord, "Keyword is not defined");
			int argc = res->second;
			if (fields.size() != argc) throw Error(_coord, "Wrong argument number");
			std::vector<double> args;
			for (auto it = fields.begin(); it != fields.end(); ++it) {
				Value val = (*it)->exec(scope);    //эти функции не принимают только double-ы
				args.push_back(val.get_double());
			}
			if (argc == 1) return Value(funcs1[_label](args[0]));
			else if (argc == 2) return Value(funcs2[_label](args[0], args[1]));
		}
	}
	return Value(0.0);
}

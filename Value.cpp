#include <cmath>
#include <sstream>
#include <algorithm>
#include <utility>

#include "Value.h"
#include "basic_HM.h"


Func::Func(const Func &f) : argv(f.argv) {
    local = f.local;
    body = new Node(*f.body);
}

Func::Func(std::vector<std::string> as, name_table nt, Node *b) :
argv(std::move(as)), local(std::move(nt)), body(b) {}


Value::BadType::BadType(Type actual, Type expected) {
    msg = "BadType exception: expected " +
          Value::type_string(expected) + " got " + Value::type_string(actual);
}

const char* Value::BadType::what() {
    return msg.c_str();
}

Value::Value() : _type(UNDEFINED) {}

Value::Value(std::array<int, 7> dim) : _type(DOUBLE) {
    _double_data = 1.0;
    _dimension = dim;
}

Value::Value(double d) : _type(DOUBLE) {
    _double_data = d;
}

Value::Value(double d, std::array<int, 7> dim) : _type(DOUBLE) {
    _double_data = d;
    _dimension = dim;
}

Value::Value(Matrix m) : _type(MATRIX) {
    _matrix_data = new Matrix(m.size());
    for (size_t i = 0; i < m.size(); ++i) {
        for (size_t j = 0; j < m[i].size(); ++j) {
            (*_matrix_data)[i].push_back(Value(m[i][j]));
        }
    }
}

Value::Value(Matrix m, std::array<int, 7> dim) : _type(MATRIX) {
    _dimension = dim;
    _matrix_data = new Matrix(m.size());
    for (size_t i = 0; i < m.size(); ++i) {
        for (size_t j = 0; j < m[i].size(); ++j) {
            (*_matrix_data)[i].push_back(Value(m[i][j]));
        }
    }
}

Value::Value(Func *f) : _type(FUNCTION) {
    _function_data = new Func(*f);
}

Value::Value(const Value &other) : _type(other._type) {
    if (_type == DOUBLE || _type == INFERRED_DOUBLE) {
        _double_data = other._double_data;
        _dimension = other._dimension;
    } else if (_type == MATRIX || _type == INFERRED_MATRIX) {
        _dimension = other._dimension;
        _matrix_data = new Matrix(other._matrix_data->size());
        for (size_t i = 0; i < _matrix_data->size(); ++i) {
            for (size_t j = 0; j < (*other._matrix_data)[i].size(); ++j) {
                (*_matrix_data)[i].push_back(Value((*other._matrix_data)[i][j]));
            }
        }
    } else if (_type == FUNCTION) {
        _function_data = new Func(*other._function_data);
    }
}

Value& Value::operator=(const Value &other) {

    if (&other != this) {
        if (_type == DOUBLE || _type == INFERRED_DOUBLE) {
            _double_data = 0.0;
            _dimension.fill(0);
        } else if (_type == MATRIX || _type == INFERRED_MATRIX) {
            delete _matrix_data;
            _dimension.fill(0);
        } else if (_type == FUNCTION) {
            delete _function_data;
        }
        _type = other._type;
        if (_type == DOUBLE || _type == INFERRED_DOUBLE) {
            _dimension = other._dimension;
            _double_data = other._double_data;
        } else if (_type == MATRIX || _type == INFERRED_MATRIX) {
            _dimension = other._dimension;
            _matrix_data = new Matrix(other._matrix_data->size());
            for (size_t i = 0; i < _matrix_data->size(); ++i) {
                for (size_t j = 0; j < (*other._matrix_data)[i].size(); ++j) {
                    (*_matrix_data)[i].push_back(Value((*other._matrix_data)[i][j]));
                }
            }
        } else if (_type == FUNCTION) {
            _function_data = new Func(*other._function_data);
        }
    }

    return *this;
}

Value::~Value() {
    if (_type == MATRIX || _type == INFERRED_MATRIX) delete _matrix_data;
    if (_type == FUNCTION) delete _function_data;
}

// Функции ниже в зависимости от типа возвращают значение или бросают исключение

double Value::get_double() const {
    if (_type != DOUBLE && _type != INFERRED_DOUBLE) {
        std::cout << "error in get_double()\n";
        throw BadType(_type, DOUBLE);
    }
    return _double_data;
}

std::array<int, 7> Value::get_dimension() const {
    if (_type != DOUBLE && _type != INFERRED_DOUBLE) {
        std::cout << "error in get_double()\n";
        throw BadType(_type, DOUBLE);
    }
    return _dimension;
}

Matrix& Value::get_matrix() const {
    if (_type != MATRIX && _type != INFERRED_MATRIX) {
        std::cout << "error in get_matrix()\n";
        throw BadType(_type, MATRIX);
    }
    return *_matrix_data;
}

Func* Value::get_function() const {
    if (_type != FUNCTION) {
        std::cout << "error in get_function()\n";
        throw BadType(_type, FUNCTION);
    }
    return _function_data;
}


Replacement::Replacement() :
tag(PLACEHOLDER), begin(0), end(0), replacement(Value(0.0, Value::dimensionless)) {}

Replacement::Replacement(Tag t, size_t b, size_t e, const Value& v) :
tag(t), begin(b), end(e), replacement(v) {}


Node::Node(Token *t) {
    _coord = t->start.start;
    _tag = t->_tag;
    if (_tag == NUMBER || _tag == IDENT || _tag == KEYWORD || _tag == DIMENSION) {
        _label = t->raw;
    } else
        _label = t->_ident;
    _priority = t_info[_tag].priority;

    if (_tag == PLACEHOLDER) {
        Node::save_rep(_coord, PLACEHOLDER, t->end.index - 2, t->end.index);
    }
}

void Node::save_rep(const Coordinate& c, Tag t, size_t a, size_t b) {
    Node::reps[c] = Replacement(t, a, b);
}

void Node::copy_defs(name_table &local, name_table *ptr) {
    if (ptr) local.insert(ptr->begin(), ptr->end());
    else local.insert(global.begin(), global.end());
}

Value &Node::lookup(const std::string& name, name_table *ptr, const Coordinate& pos) {
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

void Node::def(const std::string& name, const Value& val, name_table *ptr) {
//    std::cout << "def is invoked for name = " << name << "\n";
    if (ptr) {
        auto res = global.find(name);
        if (res == global.end()) {
            (*ptr)[name] = val;
            return;
        }
    }
    global[name] = val;
}

// Семантический анализ (проверка размерностей)
void Node::semantic_analysis() {
    if (_tag == ROOT) {
        for (auto & field : fields) {
            field->semantic_analysis();
        }
    } else {
        analyse(this, false, {}, false);
    }
}

Value Node::exec(name_table *scope = nullptr) {
    if (_tag == NUMBER) {   //если это NUMBER, то в _label записана строка с числом
        double val = std::stod(this->_label);
        return {val, Value::dimensionless};
    }
    else if (_tag == BEGINM) {  //это матрица, нужно собрать из полей Matrix
        Matrix m;   //при построении проверяется, что матрица прямоугольная и как минимум 1 х 1, поэтому здесь проверки не нужны
        for (auto & field : fields) {   //цикл по строкам
            std::vector<Value> v;
            for (auto & jt : field->fields) { //цикл по элементам строк
                v.push_back(jt->exec(scope));
            }
            m.push_back(v);
        }
        return {m};
    }
    else if (_tag == IDENT) {   //переменная
        Value x_val = Node::lookup(_label, scope, _coord);
        size_t sz = fields.size();
        if (sz == 0) {  //обычная переменная
            return x_val;
        } else {
            Matrix *m = &x_val.get_matrix();
            size_t ver = (*m).size();
            size_t hor = (*m)[0].size();

            int int_i = (int) fields[0]->exec(scope).get_double();
            if (int_i < 0) {
                throw Error(left->_coord, "Negative index");
            }
            size_t i = int_i;
            size_t j = 0;


            if (sz == 1) { //элемент вектора
                if (ver == 1) {
                    j = i;
                    i = 0;
                } else if (hor != 1) {
                    throw Error(_coord, "Can't use vector index for matrix");
                }
            } else if (sz == 2) { //элемент матрицы
                int int_j = (int) fields[1]->exec(scope).get_double();
                if (int_j < 0) {
                    throw Error(left->_coord, "Negative index");
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
        std::vector<Value> args;
        for (size_t i = 0; i < f_s; ++i) {
            args.push_back(fields[i]->exec(scope));
        }
        return Value::call(f_val, args, _coord);
    }
    else if (_tag == UADD || _tag == LPAREN) {
        return right->exec(scope);
    }
    else if (_tag == USUB) {
        return Value::usub(right->exec(scope), _coord);
    }
    else if (_tag == NOT) {
        return Value::eq(right->exec(scope), Value(0.0, Value::dimensionless), _coord);
    }
    else if (_tag == SET) {
        if (left->_tag == IDENT) {
            size_t sz = left->fields.size();
            if (sz == 0) {    //переменная
                Node::def(left->_label, right->exec(scope), scope);
            } else {    //матрица
                Value *m_val = &Node::lookup(left->_label, scope, left->_coord);
                Matrix *m = &m_val->get_matrix();
                size_t ver = (*m).size();
                size_t hor = (*m)[0].size();
                int int_i = (int) left->fields[0]->exec(scope).get_double();
                if (int_i < 0) {
                    throw Error(left->_coord, "Negative index");
                }
                size_t i = int_i;
                size_t j = 0;
                if (sz == 1) { //элемент вектора
                    if (ver == 1) {
                        j = i;
                        i = 0;
                    } else if (hor != 1) {
                        throw Error(_coord, "Bad index");
                    }
                } else if (sz == 2) { //элемент матрицы
                    int int_j = (int) left->fields[1]->exec(scope).get_double();
                    if (int_j < 0) {
                        throw Error(left->_coord, "Negative index");
                    }
                    j = int_j;
                } else {
                    throw Error(_coord, "Bad index");
                }
                if (i >= ver || j >= hor) {
                    throw Error(_coord, "Index is out of range");
                }
                (*m)[i][j] = right->exec(scope);
                return {0.0, Value::dimensionless};
            }
        }
        else if (left->_tag == FUNC) { //функция
            std::vector<std::string> ns;
            //список аргументов функции
            //при объявлении функции допустимы только IDENT в списке аргументов
            for (auto it = left->fields.begin(); it < left->fields.end(); ++it) {
                ns.push_back((*it)->_label);
            }
            //если функция объявляется глобально, ссылаться на Node из дерева нельзя
            //т.к. для каждого блока preproc строится новое, а старое удаляется
            Node *copy_of_right = new Node(*right);
            Func *f = (scope) ? new Func(ns, *scope, copy_of_right) : new Func(ns, global, copy_of_right);
            Value func_v = Value(f);
            Node::def(left->_label, func_v, scope);
        } else {
            throw Error(_coord, "Can't define this");
        }
    }
    else if (_tag == ADD) {
        return Value::plus(left->exec(scope), right->exec(scope), _coord);
    }
    else if (_tag == SUB) {
        return Value::sub(left->exec(scope), right->exec(scope), _coord);
    }
    else if (_tag == MUL) {
        return Value::mul(left->exec(scope), right->exec(scope), _coord);
    }
    else if (_tag == DIV || _tag == FRAC) {
        return Value::div(left->exec(scope), right->exec(scope), _coord);
    }
    else if (_tag == POW) {
        return Value::pow(left->exec(scope), right->exec(scope), _coord);
    }
    else if (_tag == ABS) {
        return Value::abs(right->exec(scope), _coord);
    }
    else if (_tag == EQ) {
        Value res = left->exec(scope);
        if (right->_tag == PLACEHOLDER) {
            reps[right->_coord].replacement = res;
            return {1.0, Value::dimensionless}; //равенство выполняется, вернуть 1 - нормально
        } else if (right->left != nullptr && right->left->_tag == PLACEHOLDER) {
            Value r = Value::div(res, right->right->exec(scope), _coord);
            reps[right->_coord].replacement = r;
            return {1.0, Value::dimensionless}; //равенство выполняется, вернуть 1 - нормально
        }
        return Value::eq(res, right->exec(scope), _coord);
    }
    else if (_tag == NEQ) {
        return {
            static_cast<double>(
                !Value::eq(left->exec(scope), right->exec(scope), _coord).get_double()
            )
        };
    }
    else if (_tag == LEQ) {
        return Value::le(left->exec(scope), right->exec(scope), _coord);
    }
    else if (_tag == GEQ) {
        return Value::ge(left->exec(scope), right->exec(scope), _coord);
    }
    else if (_tag == LT) {
        return Value::lt(left->exec(scope), right->exec(scope), _coord);
    }
    else if (_tag == GT) {
        return Value::gt(left->exec(scope), right->exec(scope), _coord);
    }
    else if (_tag == AND) {
        return Value::andd(left->exec(scope), right->exec(scope), _coord);
    }
    else if (_tag == OR) {
        return Value::orr(left->exec(scope), right->exec(scope), _coord);
    }
    else if (_tag == ROOT) {
        Value res(0.0);
        for (auto & field : fields) {
            res = field->exec(scope);
        }
        return res;
    }
    else if (_tag == BEGINB) {
        Value res(0.0);
        for (auto & field : fields) {
            res = field->exec(scope);
        }
        return res;
    }
    else if (_tag == BEGINC) {
        for (auto & field : fields) {
            if (!field->cond || field->cond->exec(scope).get_double() == 1.0) {
                return field->right->exec(scope);
            }
        }
    }
    else if (_tag == IF) {
        Value c_val = cond->exec(scope);
        if (c_val.get_double()) {
            return right->exec(scope);
        }
        else if (left) {
            return left->exec(scope);
        }
    }
    else if (_tag == WHILE) {
        Value res(0.0);
        while (cond->exec(scope).get_double() == 1.0) {
            res = right->exec(scope);
        }
        return res;
    }
    else if (_tag == PRODUCT) {
        Value res(0.0);
        while (cond->exec(scope).get_double() == 1.0) {
            res = right->exec(scope);
        }
        return res;
    }
    else if (_tag == TRANSP) {
        return Value::transpose(left->exec(scope));
    }
    else if (_tag == RANGE) {
        std::vector<Value> row;
        double a = left->exec(scope).get_double();
        double b = right->exec(scope).get_double();
        double d = (cond) ? Value(cond->exec(scope)).get_double() : 0.1;
        for (double x = a; x <= b; x += d) {
            row.emplace_back(x);
        }
        if (row.empty()) {
            throw Error(_coord, "Empty range");
        }
        Matrix m;
        m.push_back(row);
        return {m};
    }
    else if (_tag == GRAPHIC) {
        Value func_v = Node::lookup(_label, scope, _coord);
        Func *func = func_v.get_function();
        size_t sz = func->argv.size();
        std::vector<Value> args(sz);
        size_t ivar = 0;    //номер переменного аргумента
        bool found = false;
        for (size_t i = 0; i < sz; ++i) {
            if (fields[i]->_tag == RANGE) {
                if (!found) {
                    ivar = i;
                    found = true;
                } else {
                    throw Error(fields[i]->_coord, "More than one parameter range");
                }
            } else {
                args[i] = fields[i]->exec(scope);
            }
        }
        if (!found) {
            throw Error(_coord, "No range parameter");
        }
        Value range_v = fields[ivar]->exec(scope);
        Matrix *range = &range_v.get_matrix();

        Matrix plot;
        for (auto & it : (*range)[0]) {
            args[ivar] = it;
            double fx = Value::call(func, args, _coord).get_double();
            std::vector<Value> point = {it, Value(fx)};
            plot.push_back(point);
        }
        Value graphic(plot);
        Node::reps[_coord].replacement = graphic;
    }
    else if (_tag == KEYWORD) {
        auto res = constants.find(_label);
        if (res != constants.end()) {
            return {res->second};
        } else {
            auto result = arg_count.find(_label);
            if (result == arg_count.end()) {
                throw Error(_coord, "Keyword is not defined");
            }
            int argc = result->second;
            if (fields.size() != argc) {
                throw Error(_coord, "Wrong argument number");
            }
            std::vector<Value> args;
            for (auto & field : fields) {
                Value val = field->exec(scope);    //эти функции не принимают только double-ы
                args.push_back(val);
            }
            if (argc == 1) {
                if (_label == "\\floor" || Value::is_dimensionless(args[0])) {
                    return {funcs1[_label](args[0].get_double()), args[0].get_dimension()};
                } else {
                    std::string error = _label + " gets only dimensionless argument";
                    throw Error(_coord, error);
                }
            } else if (argc == 2) {
                return {funcs2[_label](args[0].get_double(), args[1].get_double())};
            }
        }
    }
    else if (_tag == DIMENSION) {
        auto res = dimensions.find(_label);
        return {res->second};
    }

    return {0.0, Value::dimensionless};
}

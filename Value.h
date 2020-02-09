#pragma once

#include <cassert>
#include <cmath>
#include <sstream>
#include <iomanip>
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

    constexpr const static std::array<int, 7> dimensionless = {0, 0, 0, 0, 0, 0, 0};

    static std::string type_string(Type t) {
        switch (t) {
            case DOUBLE:
                return "DOUBLE";
            case MATRIX:
                return "MATRIX";
            case FUNCTION:
                return "FUNCTION";
            default:
                assert(false);
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

        const char *what() { return msg.c_str(); }
    };

private:

    Type _type;
    union {
        double _double_data;
        std::vector<std::vector<Value>> *_matrix_data;
        Func *_function_data;
    };

    std::array<int, 7> _dimention;

public:

    static Value call(const Value &arg, std::vector<Value> arguments, Coordinate pos) {
        Func *f = arg.get_function();
        size_t sz = f->argv.size();
        for (size_t i = 0; i < sz; ++i) {
            f->local[f->argv[i]] = arguments[i];
        }
        return f->body->exec(&f->local);
    }

    Value() : _type(DOUBLE) {
        _double_data = 1.0;
        _dimention = dimensionless;
    }

    Value(std::array<int, 7> dim) : _type(DOUBLE) {
        _double_data = 1.0;
        _dimention = dim;
    }

    Value(double d) : _type(DOUBLE) {
        _double_data = d;
        _dimention = dimensionless;
    }

    Value(double d, std::array<int, 7> dim) : _type(DOUBLE) {
        _double_data = d;
        _dimention = dim;
    }

    Value(Matrix m) : _type(MATRIX) {
        _matrix_data = new Matrix(m.size());
        for (size_t i = 0; i < m.size(); ++i) {
            for (size_t j = 0; j < m[i].size(); ++j) {
                (*_matrix_data)[i].push_back(Value(m[i][j]));
            }
        }
    }

    Value(Matrix m, std::array<int, 7> dim) : _type(MATRIX) {
        _dimention = dim;
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
            _dimention = other._dimention;
        } else if (_type == MATRIX) {
            _dimention = other._dimention;
            _matrix_data = new Matrix(other._matrix_data->size());
            for (size_t i = 0; i < _matrix_data->size(); ++i) {
                for (size_t j = 0; j < (*other._matrix_data)[i].size(); ++j) {
                    (*_matrix_data)[i].push_back(Value((*other._matrix_data)[i][j]));
                }
            }
        } else {
            _function_data = new Func(*other._function_data);
        }
    }

    Value &operator=(const Value &other) {

        if (&other != this) {
            if (_type == DOUBLE) {
                _double_data = 0.0;
                _dimention.fill(0);
            } else if (_type == MATRIX) {
                delete _matrix_data;
                _dimention.fill(0);
            } else {// if (_type == FUNCTION) {
                delete _function_data;
            }
            _type = other._type;
            if (_type == DOUBLE) {
                _dimention = other._dimention;
                _double_data = other._double_data;
            } else if (_type == MATRIX) {
                _dimention = other._dimention;
                _matrix_data = new Matrix(other._matrix_data->size());
                for (size_t i = 0; i < _matrix_data->size(); ++i) {
                    for (size_t j = 0; j < (*other._matrix_data)[i].size(); ++j) {
                        (*_matrix_data)[i].push_back(Value((*other._matrix_data)[i][j]));
                    }
                }
            } else {
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

    static int count_of_dim(const std::array<int, 7> &dim) {
        int count = 0;
        for (int i : dim) {
            if (i != 0) count++;
        }
        return count;
    }

    static int count_of_pos_dim(const std::array<int, 7> &dim) {
        int count = 0;
        for (int i : dim) {
            if (i > 0) count++;
        }
        return count;
    }

    static int count_of_neg_dim(const std::array<int, 7> &dim) {
        int count = 0;
        for (int i : dim) {
            if (i < 0) count++;
        }
        return count;
    }


    friend std::string dimention_to_String(const Value &val) {
        std::string dim = "";
        int count = count_of_dim(val._dimention);
        if (count != 0) {
            for (int i = 0; i < 7; i++) {
                if (val._dimention[i] != 0) {
                    switch (i) {
                        case 0: {
                            if (val._dimention[i] == 1) {
                                dim += " \\cdot m";
                            } else {
                                dim += " \\cdot m^" + std::to_string(val._dimention[0]);
                            }
                            count--;
                            break;
                        }
                        case 1: {
                            if (val._dimention[i] == 1) {
                                dim += " \\cdot kg";
                            } else {
                                dim += " \\cdot kg^" + std::to_string(val._dimention[1]);
                            }
                            count--;
                            break;
                        }
                        case 2: {
                            if (val._dimention[i] == 1) {
                                dim += " \\cdot s";
                            } else {
                                dim += " \\cdot s^" + std::to_string(val._dimention[2]);
                            }
                            count--;
                            break;
                        }
                        case 3: {
                            if (val._dimention[i] == 1) {
                                dim += " \\cdot A";
                            } else {
                                dim += " \\cdot A^" + std::to_string(val._dimention[3]);
                            }
                            count--;
                            break;
                        }
                        case 4: {
                            if (val._dimention[i] == 1) {
                                dim += " \\cdot K";
                            } else {
                                dim += " \\cdot K^" + std::to_string(val._dimention[4]);
                            }
                            count--;
                            break;
                        }
                        case 5: {
                            if (val._dimention[i] == 1) {
                                dim += " \\cdot mol";
                            } else {
                                dim += " \\cdot mol^" + std::to_string(val._dimention[5]);
                            }
                            count--;
                            break;
                        }
                        case 6: {
                            if (val._dimention[i] == 1) {
                                dim += " \\cdot cd";
                            } else {
                                dim += " \\cdot cd^" + std::to_string(val._dimention[6]);
                            }
                            count--;
                            break;
                        }
                        default:
                            dim = "";
                            break;
                    }
                }
                if (count == 0) break;
            }
        }
        return dim;
    }

    friend std::string get_neg_dim(const Value &val, int countNeg) {
        std::string neg_dim = "";
        for (int i = 0; i < 7; i++) {
            if (val._dimention[i] < 0) {
                switch (i) {
                    case 0: {
                        if (val._dimention[i] == -1) {
                            neg_dim += "m";
                        } else {
                            neg_dim += "m^" + std::to_string(-val._dimention[0]);
                        }
                        countNeg--;
                        break;
                    }
                    case 1: {
                        if (val._dimention[i] == -1) {
                            neg_dim += "kg";
                        } else {
                            neg_dim += "kg^" + std::to_string(-val._dimention[1]);
                        }
                        countNeg--;
                        break;
                    }
                    case 2: {
                        if (val._dimention[i] == -1) {
                            neg_dim += "s";
                        } else {
                            neg_dim += "s^" + std::to_string(-val._dimention[2]);
                        }
                        countNeg--;
                        break;
                    }
                    case 3: {
                        if (val._dimention[i] == -1) {
                            neg_dim += "A";
                        } else {
                            neg_dim += "A^" + std::to_string(-val._dimention[3]);
                        }
                        countNeg--;
                        break;
                    }
                    case 4: {
                        if (val._dimention[i] == -1) {
                            neg_dim += "K";
                        } else {
                            neg_dim += "K^" + std::to_string(-val._dimention[4]);
                        }
                        countNeg--;
                        break;
                    }
                    case 5: {
                        if (val._dimention[i] == -1) {
                            neg_dim += "mol";
                        } else {
                            neg_dim += "mol^" + std::to_string(-val._dimention[5]);
                        }
                        countNeg--;
                        break;
                    }
                    case 6: {
                        if (val._dimention[i] == -1) {
                            neg_dim += "cd";
                        } else {
                            neg_dim += "cd^" + std::to_string(-val._dimention[6]);
                        }
                        countNeg--;
                        break;
                    }
                    default:
                        neg_dim = "";
                        break;
                }
                if (countNeg != 0) neg_dim += " \\cdot ";
            }
            if (countNeg == 0) {
                neg_dim += "}";
                break;
            }
        }
        return neg_dim;
    }

    friend std::string get_pos_dim(const Value &val, int countPos) {
        std::string dim = "";
        for (int i = 0; i < 7; i++) {
            if (val._dimention[i] > 0) {
                switch (i) {
                    case 0: {
                        if (val._dimention[i] == 1) {
                            dim += "m";
                        } else {
                            dim += "m^" + std::to_string(val._dimention[0]);
                        }
                        countPos--;
                        break;
                    }
                    case 1: {
                        if (val._dimention[i] == 1) {
                            dim += "kg";
                        } else {
                            dim += "kg^" + std::to_string(val._dimention[1]);
                        }
                        countPos--;
                        break;
                    }
                    case 2: {
                        if (val._dimention[i] == 1) {
                            dim += "s";
                        } else {
                            dim += "s^" + std::to_string(val._dimention[2]);
                        }
                        countPos--;
                        break;
                    }
                    case 3: {
                        if (val._dimention[i] == 1) {
                            dim += "A";
                        } else {
                            dim += "A^" + std::to_string(val._dimention[3]);
                        }
                        countPos--;
                        break;
                    }
                    case 4: {
                        if (val._dimention[i] == 1) {
                            dim += "K";
                        } else {
                            dim += "K^" + std::to_string(val._dimention[4]);
                        }
                        countPos--;
                        break;
                    }
                    case 5: {
                        if (val._dimention[i] == 1) {
                            dim += "mol";
                        } else {
                            dim += "mol^" + std::to_string(val._dimention[5]);
                        }
                        countPos--;
                        break;
                    }
                    case 6: {
                        if (val._dimention[i] == 1) {
                            dim += "cd";
                        } else {
                            dim += "cd^" + std::to_string(val._dimention[6]);
                        }
                        countPos--;
                        break;
                    }
                    default:
                        dim = "";
                        break;
                }
                if (countPos != 0) dim += " \\cdot ";
            }
            if (countPos == 0) {
                dim += "}";
                break;
            }
        }

        return dim;
    }

    friend std::string getDimention_in_frac(const Value &val) {
        std::string dim = "";
        int countPos = count_of_pos_dim(val._dimention);
        int countNeg = count_of_neg_dim(val._dimention);
        if (countNeg != 0) {
            if (countPos != 0) {
                dim = " \\cdot \\frac{";
                dim += get_pos_dim(val, countPos);
                dim += "{";
                dim += get_neg_dim(val, countNeg);
            } else {
                dim = " \\cdot \\frac{1}";
                dim += "{";
                dim += get_neg_dim(val, countNeg);
            }
        } else {
            dim = dimention_to_String(val);
        }
        return dim;
    }

    static std::string double_to_String(double d) {

        std::stringstream streamObj;
        streamObj << std::fixed;
        streamObj << std::setprecision(5);
        double short_d, f;
        streamObj << d;
        streamObj >> short_d;
        std::string res = "";

        if (std::modf(d, &f) == 0) {
            return std::to_string((int) short_d);
        } else {
            double mod = modf(short_d, &f);
            long mod2 = (long) (mod * 100000);
            return std::to_string((long) (short_d)) + "." + std::to_string(mod2);
        }
    }

    friend std::string to_string(const Value &val) {
        if (val._type == DOUBLE) {
            return double_to_String(val._double_data) + getDimention_in_frac(val);
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
                } else break;
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
            std::cout << "error in get_double()\n";
            throw BadType(_type, DOUBLE);
        }
        return _double_data;
    }

    std::array<int, 7> get_dimention() const {
        if (_type != DOUBLE) {
            std::cout << "error in get_double()\n";
            throw BadType(_type, DOUBLE);
        }
        return _dimention;
    }

    Matrix &get_matrix() const {
        if (_type != MATRIX) {
            std::cout << "error in get_matrix()\n";
            throw BadType(_type, MATRIX);
        }
        return *_matrix_data;
    }

    Func *get_function() const {
        if (_type != FUNCTION) {
            std::cout << "error in get_function()\n";
            throw BadType(_type, FUNCTION);
        }
        return _function_data;
    }

    friend bool is_equal_dim(const Value &left, const Value &right) {
        for (int i = 0; i < 7; i++) {
            if (left._dimention[i] != right._dimention[i]) return false;
        }
        return true;
    }

    static bool is_dimensionless(const Value &value) {
        for (int i: value._dimention) {
            if (i != 0) return false;
        }
        return true;
    }

    static Value plus(const Value &left, const Value &right, Coordinate pos) {
//        std::cout << "plus\n";
        if (left._type == DOUBLE) { //если right - не DOUBLE, сработает исключение
            if (is_equal_dim(left, right)) {
                return Value(left.get_double() + right.get_double(), left._dimention);
            } else {
                throw Error(pos, "the addition of different dimensions");
            }
        } else if (left._type == MATRIX) {
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
            } else {
                throw Error(pos, "Dimension mismatch");
            }
        }
        throw Error(pos, "Addition by function");
    }

    static Value usub(const Value &arg, Coordinate pos) {
        if (arg._type == DOUBLE) {
            return Value(-arg.get_double(), arg._dimention);
        } else if (arg._type == MATRIX) {
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
            if (is_equal_dim(left, right)) {
                return Value(left.get_double() - right.get_double(), left._dimention);
            } else {
                throw Error(pos, "the subtraction of different dimensions");
            }
        } else if (left._type == MATRIX) {
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
            } else {
                throw Error(pos, "Dimension mismatch");
            }
        }
        throw Error(pos, "Substitution by function");
    }

    static Value mul(const Value &left, const Value &right, Coordinate pos) {

//        std::cout << "\nMUL\n";
//        std::cout << "left.double = " << left.get_double() << "\n";
//        std::cout << "right.double = " << right.get_double() << "\n";
//        std::cout << "left dim = ";
//        for (int i: left._dimention) std::cout << i << " ";
//        std::cout << "\n";
//        std::cout << "right dim = ";
//        for (int i: right._dimention) std::cout << i << " ";
//        std::cout << "\n\n";
        if (left._type == DOUBLE) {
            if (right._type == DOUBLE) {
//                std::cout << "right._type == DOUBLE\n";
                std::array<int, 7> dim{};
                for (int i = 0; i < 7; i++) {
                    dim[i] = left._dimention[i] + right._dimention[i];
                }
//                std::cout << "new DIM = ";
//                for (int i = 0; i < 7; i++) {
//                    std::cout << dim[i] << " ";
//                }
//                std::cout << "\n\n";
                return Value(left.get_double() * right.get_double(), dim);

            } else if (right._type == MATRIX) {
                Matrix *r = &right.get_matrix();
                Matrix mult((*r).size());
                for (size_t i = 0; i < (*r).size(); ++i) {
                    for (size_t j = 0; j < (*r)[0].size(); ++j) {
                        mult[i].push_back(mul(left, (*r)[i][j], pos));
                    }
                }
                return Value(mult);
            }
        } else if (left._type == MATRIX) {
            if (right._type == DOUBLE) {
                return mul(right, left, pos);
            } else if (right._type == MATRIX) {
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
                else if (l_vert == 1 && r_vert == 1) {    //строка*строка => строка*столбец
                    Value res = Value::mul(*l, Value::transp(*r, pos), pos);    //если длины строк равны, mul выполнится
                    return Value(res.get_matrix()[0][0]);
                } else if (l_hor == 1 && r_hor == 1) {    //столбец*столбец => строка*столбец
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
                std::array<int, 7> dim;
                for (int i = 0; i < 7; i++) {
                    dim[i] = left._dimention[i] - right._dimention[i];
                }
                return Value(left.get_double() / q, dim);

            } else if (right._type == MATRIX) {
                throw Error(pos, "Division by matrix");
            }
        } else if (left._type == MATRIX) {
            if (right._type == DOUBLE) {
                double q = right.get_double();
                if (q == 0.0) {
                    throw Error(pos, "Division by zero");
                }
                return mul(Value(1.0 / q), left, pos);
            } else if (right._type == MATRIX) {

                throw Error(pos, "Division by matrix");
            }
        }
        throw Error(pos, "Division by function");
    }

    //static Value call(const Value &arg, std::vector<Value> arguments, Coordinate pos);

    static Value eq(const Value &left, const Value &right, Coordinate pos) {
        if (left._type != right._type) {
            return Value(0.0, dimensionless);  //точно не равны
        }
        if (left._type == DOUBLE) {
            if (is_equal_dim(left, right)) {
                return Value(left.get_double() == right.get_double());
            } else {
                throw Error(pos, "Comparison of different dimensions");
            }
        }
        if (left._type == MATRIX) {
            Matrix *l = &left.get_matrix();
            Matrix *r = &right.get_matrix();
            if (l->size() == r->size() && (*l)[0].size() == (*r)[0].size()) {
                for (size_t i = 0; i < l->size(); ++i) {
                    for (size_t j = 0; j < (*l)[0].size(); ++j) {
                        Value x = eq((*l)[i][j], (*r)[i][j], pos);
                        if (x.get_double() == 0.0) return Value(0.0, dimensionless);
                    }
                }
                return Value(1.0, dimensionless);
            }
        }
        //функции не понятно как сравнивать
        return Value(0.0, dimensionless);
    }

    static Value le(const Value &left, const Value &right, Coordinate pos) {
        if (is_equal_dim(left, right)) {
            return Value(left.get_double() <= right.get_double());  //иначе не имеет смысла
        } else {
            throw Error(pos, "Comparison of different dimensions");
        }
    }

    static Value ge(const Value &left, const Value &right, Coordinate pos) {
        if (is_equal_dim(left, right)) {
            return Value(left.get_double() >= right.get_double());
        } else {
            throw Error(pos, "Comparison of different dimensions");
        }
    }

    static Value lt(const Value &left, const Value &right, Coordinate pos) {
        if (is_equal_dim(left, right)) {
            return Value(left.get_double() < right.get_double());
        } else {
            throw Error(pos, "Comparison of different dimensions");
        }
    }

    static Value gt(const Value &left, const Value &right, Coordinate pos) {
        if (is_equal_dim(left, right)) {
            return Value(left.get_double() > right.get_double());
        } else {
            throw Error(pos, "Comparison of different dimensions");
        }
    }

    static std::array<int, 7> mul_dimention(std::array<int, 7> dim, double n) {
        std::array<int, 7> tmp_dim = dim;
        for (int i = 0; i < 7; i++) tmp_dim[i] = dim[i] * n;

        return tmp_dim;
    }

    static Value pow(const Value &left, const Value &right, Coordinate pos) {
        double floor;
        if (is_dimensionless(right)) {
            if (Value::is_dimensionless(left)) {
                return Value(std::pow(left.get_double(), right.get_double()),
                             mul_dimention(left.get_dimention(), right.get_double()));
            } else if (modf(right.get_double(), &floor) == 0.0) {
                return Value(std::pow(left.get_double(), right.get_double()),
                             mul_dimention(left.get_dimention(), right.get_double()));
            } else throw Error(pos, "Power of float number is not allowed");
        } else {
            throw Error(pos, "Power of dimensional number is not defined");
        }
    }

    static Value abs(const Value &right, Coordinate pos) {
        return Value(std::abs(right.get_double()), right._dimention);
    }

    static Value andd(const Value &left, const Value &right, Coordinate pos) {
        return Value(left.get_double() && right.get_double());
    }

    static Value orr(const Value &left, const Value &right, Coordinate pos) {
        return Value(left.get_double() || right.get_double());
    }

    //static Value call(const Value & arg, std::vector<Value> arguments, name_table * scope, Coordinate pos);
//	static Value call(const Value & arg, std::vector<Value> arguments, Coordinate pos);

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

    Replacement() : tag(PLACEHOLDER), begin(0), end(0), replacement(Value(0.0, Value::dimensionless)) {}

    Replacement(Tag t, size_t b, size_t e, Value v = Value(0.0, Value::dimensionless))
            : tag(t), begin(b), end(e), replacement(v) {}
} Replacement;

Node::Node(Token *t) {
    _coord = t->start.start;
    _tag = t->_tag;
    if (_tag == NUMBER || _tag == IDENT || _tag == KEYWORD || _tag == DIMENTION) {
        _label = t->raw;
    } else
        _label = t->_ident;
    _priority = t_info[_tag].priority;

    if (_tag == PLACEHOLDER) {
        Node::save_rep(_coord, PLACEHOLDER, t->end.index - 2, t->end.index);
//        std::cout << "_coord = " << to_string(_coord) << std::endl;
//        std::cout << "t->start.index + strlen(placeholder = " << t->start.index + strlen("\\placeholder") << std::endl;
//        std::cout << "t->end.index = " << t->end.index << std::endl;
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


void Node::def(std::string name, Value val, name_table *ptr) {
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

Value Node::exec(name_table *scope = nullptr) {
    if (_tag == NUMBER) {   //если это NUMBER, то в _label записана строка с числом
        double val = std::stod(this->_label);
        return Value(val, Value::dimensionless);
    } else if (_tag == BEGINM) {  //это матрица, нужно собрать из полей Matrix
        Matrix m;   //при построении проверяется, что матрица прямоугольная и как минимум 1 х 1, поэтому здесь проверки не нужны
        for (auto it = fields.begin(); it != fields.end(); ++it) {   //цикл по строкам
            std::vector<Value> v;
            for (auto jt = (*it)->fields.begin(); jt != (*it)->fields.end(); ++jt) { //цикл по элементам строк
                v.push_back((*jt)->exec(scope));
            }
            m.push_back(v);
        }
        return Value(m);
    } else if (_tag == IDENT) {   //переменная
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
                throw new Error(left->_coord, "Negative index");
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
                    throw new Error(left->_coord, "Negative index");
                }
                j = int_j;
            }
            if (i >= ver || j >= hor) {
                throw Error(_coord, "Index is out of range");
            }
            return (*m)[i][j];
        }

    } else if (_tag == FUNC) {  //вызов функции
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
    } else if (_tag == UADD || _tag == LPAREN) return right->exec(scope);
    else if (_tag == USUB) return Value::usub(right->exec(scope), _coord);
    else if (_tag == NOT) {
        return Value::eq(right->exec(scope), Value(0.0, Value::dimensionless), _coord);
    } else if (_tag == SET) {
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
                    throw new Error(left->_coord, "Negative index");
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
                        throw new Error(left->_coord, "Negative index");
                    }
                    j = int_j;
                } else {
                    throw Error(_coord, "Bad index");
                }
                if (i >= ver || j >= hor) {
                    throw Error(_coord, "Index is out of range");
                }
                (*m)[i][j] = right->exec(scope);
                return Value(0.0, Value::dimensionless);
            }
        } else if (left->_tag == FUNC) { //функция
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
        } else throw Error(_coord, "Cant't define this");
    } else if (_tag == ADD) return Value::plus(left->exec(scope), right->exec(scope), _coord);
    else if (_tag == SUB) return Value::sub(left->exec(scope), right->exec(scope), _coord);
    else if (_tag == MUL) {
        return Value::mul(left->exec(scope), right->exec(scope), _coord);
    } else if (_tag == DIV || _tag == FRAC) return Value::div(left->exec(scope), right->exec(scope), _coord);
    else if (_tag == POW) return Value::pow(left->exec(scope), right->exec(scope), _coord);
    else if (_tag == ABS) return Value::abs(right->exec(scope), _coord);
    else if (_tag == EQ) {
        Value res = left->exec(scope);
        if (right->_tag == PLACEHOLDER) {
            reps[right->_coord].replacement = res;
            return Value(1.0, Value::dimensionless); //равенство выполняется, вернуть 1 - нормально
        } else if (right->left != nullptr && right->left->_tag == PLACEHOLDER) {
            Value r = Value::div(res, right->right->exec(scope), _coord);
            reps[right->_coord].replacement = r;
            return Value(1.0, Value::dimensionless); //равенство выполняется, вернуть 1 - нормально
        }
        return Value::eq(res, right->exec(scope), _coord);
    } else if (_tag == NEQ) {
        return Value(!Value::eq(left->exec(scope), right->exec(scope), _coord).get_double());
    } else if (_tag == LEQ) return Value::le(left->exec(scope), right->exec(scope), _coord);
    else if (_tag == GEQ) return Value::ge(left->exec(scope), right->exec(scope), _coord);
    else if (_tag == LT) return Value::lt(left->exec(scope), right->exec(scope), _coord);
    else if (_tag == GT) return Value::gt(left->exec(scope), right->exec(scope), _coord);
    else if (_tag == AND) return Value::andd(left->exec(scope), right->exec(scope), _coord);
    else if (_tag == OR) return Value::orr(left->exec(scope), right->exec(scope), _coord);
    else if (_tag == ROOT) {
        Value res(0.0);
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            res = (*it)->exec(scope);
        }
        return res;
    } else if (_tag == BEGINB) {
        Value res(0.0);
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            res = (*it)->exec(scope);
        }
        return res;
    } else if (_tag == BEGINC) {
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            if (!(*it)->cond || (*it)->cond->exec(scope).get_double() == 1.0) {
                return (*it)->right->exec(scope);
            }
        }
    } else if (_tag == IF) {
        Value c_val = cond->exec(scope);
        if (c_val.get_double()) return right->exec(scope);
        else if (left) return left->exec(scope);
    } else if (_tag == WHILE) {
        Value res(0.0);
//        int i = 0;
//		while (i++ < 10 && cond->exec(scope).get_double() == 1.0) {
        while (cond->exec(scope).get_double() == 1.0) {
            res = right->exec(scope);
        }
        return res;
    } else if (_tag == PRODUCT) {
        Value res(0.0);
        while (cond->exec(scope).get_double() == 1.0) {
            res = right->exec(scope);
            if (Value::is_dimensionless(res)) {
            } else throw Error(_coord, "Element of product is not allowed to be dimensional");
        }
        return res;
    } else if (_tag == TRANSP) {
        return Value::transp(left->exec(scope), _coord);
    } else if (_tag == RANGE) {
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
    } else if (_tag == GRAPHIC) {
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
        for (auto it = (*range)[0].begin(); it != (*range)[0].end(); ++it) {
            args[ivar] = *it;
            double fx = func_v.call(func, args, _coord).get_double();
            std::vector<Value> point = {*it, Value(fx)};
            plot.push_back(point);
        }
        Value graphic(plot);
        Node::reps[_coord].replacement = graphic;
    } else if (_tag == KEYWORD) {
        auto res = constants.find(_label);
        if (res != constants.end()) {
            return Value(res->second);
        } else {
            auto res = arg_count.find(_label);
            if (res == arg_count.end()) throw Error(_coord, "Keyword is not defined");
            int argc = res->second;
            if (fields.size() != argc) throw Error(_coord, "Wrong argument number");
            std::vector<Value> args;
            for (auto it = fields.begin(); it != fields.end(); ++it) {
                Value val = (*it)->exec(scope);    //эти функции не принимают только double-ы
                args.push_back(val);
            }
            if (argc == 1) {
                if (_label == "\\floor" || !Value::is_dimensionless(args[0])) {
                    return Value(funcs1[_label](args[0].get_double()), args[0].get_dimention());
                } else {
                    std::string error = _label + " gets only dimensionless argument";
                    throw Error(_coord, error);
                }
            } else if (argc == 2) return Value(funcs2[_label](args[0].get_double(), args[1].get_double()));
        }
    } else if (_tag == DIMENTION) {
        auto res = dimentions.find(_label);
        if (res != dimentions.end()) {
            return Value(res->second);
        } else throw Error(_coord, "This unit is not basic");
    }
    return Value(0.0, Value::dimensionless);
}

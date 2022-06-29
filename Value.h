#pragma once

#include <cassert>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <utility>
#include "Node.h"
#include "Error.h"


typedef struct Func {
    std::vector<std::string> argv;
    name_table local;
    Node* body;

    Func(const Func &f);

    Func(std::vector<std::string> as, name_table nt, Node *b);
} Func;

typedef std::vector<std::vector<Value>> Matrix;

class Value {
public:
    typedef enum Type {
        DOUBLE, MATRIX, FUNCTION, UNDEFINED, INFERRED_DOUBLE, INFERRED_MATRIX
    } Type;

    Type _type;
    std::array<int, 7> _dimension = dimensionless;

    constexpr const static std::array<int, 7> dimensionless = {0, 0, 0, 0, 0, 0, 0};

    static std::string type_string(Type t) {
        switch (t) {
            case DOUBLE:
                return "DOUBLE";
            case MATRIX:
                return "MATRIX";
            case FUNCTION:
                return "FUNCTION";
            case UNDEFINED:
                return "UNDEFINED";
            case INFERRED_DOUBLE:
                return "INFERRED_DOUBLE";
            case INFERRED_MATRIX:
                return "INFERRED_MATRIX";
            default:
                assert(false);
        }
    }

    class BadType : public std::exception {
    private:
        std::string msg;
    public:
        BadType(Type actual, Type expected);

        const char *what();
    };

private:
    union {
        double _double_data;
        std::vector<std::vector<Value>> *_matrix_data;
        Func *_function_data;
    };

public:

    static Value call(const Value &arg, std::vector<Value> arguments, const Coordinate& pos) {
        Func *f = arg.get_function();
        size_t sz = f->argv.size();
        for (size_t i = 0; i < sz; ++i) {
            f->local[f->argv[i]] = arguments[i];
        }
        return f->body->exec(&f->local);
    }

    Value();

    Value(std::array<int, 7> dim);

    Value(double d);

    Value(double d, std::array<int, 7> dim);

    Value(Matrix m);

    Value(Matrix m, std::array<int, 7> dim);

    Value(Func *f);

    Value(const Value &other);

    Value &operator=(const Value &other);

    ~Value();

    friend std::string to_plot(const Value &matr) {
        if (matr._type == MATRIX || matr._type == INFERRED_MATRIX) {
            std::string res;
            Matrix *m = &matr.get_matrix();
            for (auto & it : *m) {
                res += "(" + std::to_string(it[0].get_double()) + ","
                       + std::to_string(it[1].get_double()) + ")\n";
            }
            return res;
        }

        return "";
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

    friend std::string dimension_to_String(const Value &val) {
        std::string dim;
        int count = count_of_dim(val._dimension);
        if (count != 0) {
            for (int i = 0; i < 7; i++) {
                if (val._dimension[i] != 0) {
                    switch (i) {
                        case 0: {
                            if (val._dimension[i] == 1) {
                                dim += " \\cdot m";
                            } else {
                                dim += " \\cdot m^" + std::to_string(val._dimension[0]);
                            }
                            count--;
                            break;
                        }
                        case 1: {
                            if (val._dimension[i] == 1) {
                                dim += " \\cdot kg";
                            } else {
                                dim += " \\cdot kg^" + std::to_string(val._dimension[1]);
                            }
                            count--;
                            break;
                        }
                        case 2: {
                            if (val._dimension[i] == 1) {
                                dim += " \\cdot s";
                            } else {
                                dim += " \\cdot s^" + std::to_string(val._dimension[2]);
                            }
                            count--;
                            break;
                        }
                        case 3: {
                            if (val._dimension[i] == 1) {
                                dim += " \\cdot A";
                            } else {
                                dim += " \\cdot A^" + std::to_string(val._dimension[3]);
                            }
                            count--;
                            break;
                        }
                        case 4: {
                            if (val._dimension[i] == 1) {
                                dim += " \\cdot K";
                            } else {
                                dim += " \\cdot K^" + std::to_string(val._dimension[4]);
                            }
                            count--;
                            break;
                        }
                        case 5: {
                            if (val._dimension[i] == 1) {
                                dim += " \\cdot mol";
                            } else {
                                dim += " \\cdot mol^" + std::to_string(val._dimension[5]);
                            }
                            count--;
                            break;
                        }
                        case 6: {
                            if (val._dimension[i] == 1) {
                                dim += " \\cdot cd";
                            } else {
                                dim += " \\cdot cd^" + std::to_string(val._dimension[6]);
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
        std::string neg_dim;
        for (int i = 0; i < 7; i++) {
            if (val._dimension[i] < 0) {
                switch (i) {
                    case 0: {
                        if (val._dimension[i] == -1) {
                            neg_dim += "m";
                        } else {
                            neg_dim += "m^" + std::to_string(-val._dimension[0]);
                        }
                        countNeg--;
                        break;
                    }
                    case 1: {
                        if (val._dimension[i] == -1) {
                            neg_dim += "kg";
                        } else {
                            neg_dim += "kg^" + std::to_string(-val._dimension[1]);
                        }
                        countNeg--;
                        break;
                    }
                    case 2: {
                        if (val._dimension[i] == -1) {
                            neg_dim += "s";
                        } else {
                            neg_dim += "s^" + std::to_string(-val._dimension[2]);
                        }
                        countNeg--;
                        break;
                    }
                    case 3: {
                        if (val._dimension[i] == -1) {
                            neg_dim += "A";
                        } else {
                            neg_dim += "A^" + std::to_string(-val._dimension[3]);
                        }
                        countNeg--;
                        break;
                    }
                    case 4: {
                        if (val._dimension[i] == -1) {
                            neg_dim += "K";
                        } else {
                            neg_dim += "K^" + std::to_string(-val._dimension[4]);
                        }
                        countNeg--;
                        break;
                    }
                    case 5: {
                        if (val._dimension[i] == -1) {
                            neg_dim += "mol";
                        } else {
                            neg_dim += "mol^" + std::to_string(-val._dimension[5]);
                        }
                        countNeg--;
                        break;
                    }
                    case 6: {
                        if (val._dimension[i] == -1) {
                            neg_dim += "cd";
                        } else {
                            neg_dim += "cd^" + std::to_string(-val._dimension[6]);
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
        std::string dim;
        for (int i = 0; i < 7; i++) {
            if (val._dimension[i] > 0) {
                switch (i) {
                    case 0: {
                        if (val._dimension[i] == 1) {
                            dim += "m";
                        } else {
                            dim += "m^" + std::to_string(val._dimension[0]);
                        }
                        countPos--;
                        break;
                    }
                    case 1: {
                        if (val._dimension[i] == 1) {
                            dim += "kg";
                        } else {
                            dim += "kg^" + std::to_string(val._dimension[1]);
                        }
                        countPos--;
                        break;
                    }
                    case 2: {
                        if (val._dimension[i] == 1) {
                            dim += "s";
                        } else {
                            dim += "s^" + std::to_string(val._dimension[2]);
                        }
                        countPos--;
                        break;
                    }
                    case 3: {
                        if (val._dimension[i] == 1) {
                            dim += "A";
                        } else {
                            dim += "A^" + std::to_string(val._dimension[3]);
                        }
                        countPos--;
                        break;
                    }
                    case 4: {
                        if (val._dimension[i] == 1) {
                            dim += "K";
                        } else {
                            dim += "K^" + std::to_string(val._dimension[4]);
                        }
                        countPos--;
                        break;
                    }
                    case 5: {
                        if (val._dimension[i] == 1) {
                            dim += "mol";
                        } else {
                            dim += "mol^" + std::to_string(val._dimension[5]);
                        }
                        countPos--;
                        break;
                    }
                    case 6: {
                        if (val._dimension[i] == 1) {
                            dim += "cd";
                        } else {
                            dim += "cd^" + std::to_string(val._dimension[6]);
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

    friend std::string getDimension_in_frac(const Value &val) {
        std::string dim;
        int countPos = count_of_pos_dim(val._dimension);
        int countNeg = count_of_neg_dim(val._dimension);
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
            dim = dimension_to_String(val);
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
        std::string res;

        if (std::modf(d, &f) == 0) {
            return std::to_string((int) short_d);
        } else {
            double mod = modf(short_d, &f);
            long mod2 = (long) (mod * 100000);
            return std::to_string((long) (short_d)) + "." + std::to_string(mod2);
        }
    }

    friend std::string to_string(const Value &val) {
        if (val._type == DOUBLE || val._type == INFERRED_DOUBLE) {
            return double_to_String(val._double_data) + getDimension_in_frac(val);
        }
        if (val._type == MATRIX || val._type == INFERRED_MATRIX) {
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
        if (val._type == UNDEFINED) {
            return "undefined";
        }

        return "";
    }

    double get_double() const;

    std::array<int, 7> get_dimension() const;

    Matrix& get_matrix() const;

    Func* get_function() const;

    static bool is_equal_dim(const Value &left, const Value &right) {
        for (int i = 0; i < 7; i++) {
            if (left._dimension[i] != right._dimension[i]) {
                return false;
            }
        }
        return true;
    }

    static bool is_dimensionless(const Value &value) {
        for (int i : value._dimension) {
            if (i != 0) {
                return false;
            }
        }
        return true;
    }

    static Value plus(const Value &left, const Value &right, const Coordinate& pos) {
        if (left._type == DOUBLE || left._type == INFERRED_DOUBLE) { //если right - не DOUBLE, сработает исключение
            return {left.get_double() + right.get_double(), left._dimension};
        } else if (left._type == MATRIX || left._type == INFERRED_MATRIX) {
            Matrix *l = &left.get_matrix();
            Matrix *r = &right.get_matrix();
            if ((*l).size() == (*r).size() && (*l)[0].size() == (*r)[0].size()) {
                Matrix sum(l->size());
                for (size_t i = 0; i < (*l).size(); ++i) {
                    for (size_t j = 0; j < (*l)[0].size(); ++j) {
                        sum[i].push_back(plus((*l)[i][j], (*r)[i][j], pos));
                    }
                }
                return {sum};
            } else {
                throw Error(pos, "Matrix dimensions mismatch");
            }
        }
        throw Error(pos, "Addition cannot be done");
    }

    static Value usub(const Value &arg, const Coordinate& pos) {
        if (arg._type == DOUBLE || arg._type == INFERRED_DOUBLE) {
            return {-arg.get_double(), arg._dimension};
        } else if (arg._type == MATRIX || arg._type == INFERRED_MATRIX) {
            Matrix *a = &arg.get_matrix();
            Matrix res(a->size());
            for (size_t i = 0; i < res.size(); ++i) {
                for (size_t j = 0; j < res[i].size(); ++j) {
                    res[i].push_back(usub((*a)[i][j], pos));
                }
            }
            return {res};
        }
        throw Error(pos, "Substitution cannot be done");
    }

    static Value sub(const Value &left, const Value &right, const Coordinate& pos) {
        if (left._type == DOUBLE || left._type == INFERRED_DOUBLE) {
            return {left.get_double() - right.get_double(), left._dimension};
        } else if (left._type == MATRIX || left._type == INFERRED_MATRIX) {
            Matrix *l = &left.get_matrix();
            Matrix *r = &right.get_matrix();
            if ((*l).size() == (*r).size() && (*l)[0].size() == (*r)[0].size()) {
                Matrix dif(l->size());
                for (size_t i = 0; i < (*l).size(); ++i) {
                    for (size_t j = 0; j < (*l)[0].size(); ++j) {
                        dif[i].push_back(sub((*l)[i][j], (*r)[i][j], pos));
                    }
                }
                return {dif};
            } else {
                throw Error(pos, "Matrix dimensions mismatch");
            }
        }
        throw Error(pos, "Substitution cannot be done");
    }

    static Value mul(const Value &left, const Value &right, const Coordinate& pos) {
        if (left._type == DOUBLE || left._type == INFERRED_DOUBLE) {
            if (right._type == DOUBLE || right._type == INFERRED_DOUBLE) {
                std::array<int, 7> dim{};
                for (int i = 0; i < 7; i++) {
                    dim[i] = left._dimension[i] + right._dimension[i];
                }
                return {left.get_double() * right.get_double(), dim};

            } else if (right._type == MATRIX || right._type == INFERRED_MATRIX) {
                Matrix *r = &right.get_matrix();
                Matrix mult((*r).size());
                for (size_t i = 0; i < (*r).size(); ++i) {
                    for (size_t j = 0; j < (*r)[0].size(); ++j) {
                        mult[i].push_back(mul(left, (*r)[i][j], pos));
                    }
                }
                return {mult};
            }
        } else if (left._type == MATRIX || left._type == INFERRED_MATRIX) {
            if (right._type == DOUBLE || right._type == INFERRED_DOUBLE) {
                return mul(right, left, pos);
            } else if (right._type == MATRIX || right._type == INFERRED_MATRIX) {
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
                    return {mult};
                }

                //скалярное произведение
                else if (l_vert == 1 && r_vert == 1) {    //строка*строка => строка*столбец
                    Value res = Value::mul(*l, Value::transpose(*r), pos);    //если длины строк равны, mul выполнится
                    return {res.get_matrix()[0][0]};
                } else if (l_hor == 1 && r_hor == 1) {    //столбец*столбец => строка*столбец
                    Value res = Value::mul(Value::transpose(*l), *r, pos);
                    return {res.get_matrix()[0][0]};
                }
                throw Error(pos, "Matrix/vector dimensions mismatch");
            }
        }
        throw Error(pos, "Multiplication cannot be done");
    }

    static Value div(const Value &left, const Value &right, const Coordinate& pos) {
        if (left._type == DOUBLE || left._type == INFERRED_DOUBLE) {
            if (right._type == DOUBLE || right._type == INFERRED_DOUBLE) {
                double q = right.get_double();
                if (q == 0.0) {
                    throw Error(pos, "Division by zero");
                }
                std::array<int, 7> dim{};
                for (int i = 0; i < 7; i++) {
                    dim[i] = left._dimension[i] - right._dimension[i];
                }
                return {left.get_double() / q, dim};
            }
        } else if (left._type == MATRIX || left._type == INFERRED_MATRIX) {
            if (right._type == DOUBLE || right._type == INFERRED_DOUBLE) {
                double q = right.get_double();
                if (q == 0.0) {
                    throw Error(pos, "Division by zero");
                }
                return mul(Value(1.0 / q), left, pos);
            }
        }

        throw Error(pos, "Division cannot be done");
    }

    static Value eq(const Value &left, const Value &right, const Coordinate& pos) {
        if (!(
            (left._type == DOUBLE || right._type == INFERRED_DOUBLE) &&
            (right._type == DOUBLE || right._type == INFERRED_DOUBLE)
            ||
            (left._type == MATRIX || right._type == INFERRED_MATRIX) &&
            (right._type == MATRIX || right._type == INFERRED_MATRIX)
        )) {
            return {0.0, dimensionless};  //точно не равны
        }
        if (left._type == DOUBLE || left._type == INFERRED_DOUBLE) {
            return {static_cast<double>(left.get_double() == right.get_double())};
        }
        if (left._type == MATRIX || left._type == INFERRED_MATRIX) {
            Matrix *l = &left.get_matrix();
            Matrix *r = &right.get_matrix();
            if (l->size() == r->size() && (*l)[0].size() == (*r)[0].size()) {
                for (size_t i = 0; i < l->size(); ++i) {
                    for (size_t j = 0; j < (*l)[0].size(); ++j) {
                        Value x = eq((*l)[i][j], (*r)[i][j], pos);
                        if (x.get_double() == 0.0) return {0.0, dimensionless};
                    }
                }
                return {1.0, dimensionless};
            }
        }

        //функции не понятно как сравнивать
        return {0.0, dimensionless};
    }

    static Value le(const Value &left, const Value &right, const Coordinate& pos) {
        return {static_cast<double>(left.get_double() <= right.get_double())};  //иначе не имеет смысла
    }

    static Value ge(const Value &left, const Value &right, const Coordinate& pos) {
        return {static_cast<double>(left.get_double() >= right.get_double())};
    }

    static Value lt(const Value &left, const Value &right, const Coordinate& pos) {
        return {static_cast<double>(left.get_double() < right.get_double())};
    }

    static Value gt(const Value &left, const Value &right, const Coordinate& pos) {
        return {static_cast<double>(left.get_double() > right.get_double())};
    }

    static std::array<int, 7> mul_dimension(std::array<int, 7> dim, double n) {
        std::array<int, 7> tmp_dim = dim;
        for (int i = 0; i < 7; i++) {
            tmp_dim[i] = dim[i] * n;
        }
        return tmp_dim;
    }

    static Value pow(const Value &left, const Value &right, const Coordinate& pos) {
        double floor;

        if (Value::is_dimensionless(left)) {
            return {
                    std::pow(left.get_double(), right.get_double()),
                    mul_dimension(left.get_dimension(), right.get_double())
                  };
        } else if (modf(right.get_double(), &floor) == 0.0) {
            return {
                    std::pow(left.get_double(),
                    right.get_double()),
                  mul_dimension(left.get_dimension(), right.get_double())
                  };
        } else {
            throw Error(pos, "Power of float number is not allowed");
        }
    }

    static Value abs(const Value &right, const Coordinate& pos) {
        return {std::abs(right.get_double()), right._dimension};
    }

    static Value andd(const Value &left, const Value &right, const Coordinate& pos) {
        return {static_cast<double>(left.get_double() && right.get_double())};
    }

    static Value orr(const Value &left, const Value &right, const Coordinate& pos) {
        return {static_cast<double>(left.get_double() || right.get_double())};
    }

    static Value transpose(const Value &matrix) {
        Matrix *m = &matrix.get_matrix();
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
        return {mt};
    }

    // Проверка идентичности размерностей
    static bool check_dimensions(const int* first, const int* second) {
        for (int i = 0; i < 7; i++) {
            if (first[i] != second[i]) {
                return false;
            }
        }

        return true;
    }

    static bool check_dimensions(const std::array<int, 7> first, const std::array<int, 7> second) {
        for (int i = 0; i < 7; i++) {
            if (first[i] != second[i]) {
                return false;
            }
        }

        return true;
    }

    static std::array<int, 7> sum_dimensions(const std::array<int, 7> first, const std::array<int, 7> second) {
        std::array<int, 7> new_dims = std::array<int, 7>();

        for (int i = 0; i < 7; i++) {
            new_dims[i] = first[i] + second[i];
        }

        return new_dims;
    }

    static std::array<int, 7> sub_dimensions(const std::array<int, 7> first, const std::array<int, 7> second) {
        std::array<int, 7> new_dims = std::array<int, 7>();

        for (int i = 0; i < 7; i++) {
            new_dims[i] = first[i] - second[i];
        }

        return new_dims;
    }

    static std::array<int, 7> mul_dimensions(const std::array<int, 7> dims, int degree) {
        std::array<int, 7> new_dims = std::array<int, 7>();

        for (int i = 0; i < 7; i++) {
            new_dims[i] = dims[i] * degree;
        }

        return new_dims;
    }

    static bool is_dimensionless(const int* dims) {
        for (int i = 0; i < 7; i++) {
            if (dims[i] != 0) {
                return false;
            }
        }

        return true;
    }

    static bool is_matrix_equals_dims(const Matrix& first, const Matrix& second) {
        if (first.size() != second.size()) {
            return false;
        }

        if (first[0].size() != second[0].size()) {
            return false;
        }

        return true;
    }
};

typedef struct Replacement {
    Tag tag;
    size_t begin;
    size_t end;
    Value replacement;

    Replacement();

    Replacement(Tag, size_t, size_t, const Value& = Value(0.0, Value::dimensionless));
} Replacement;
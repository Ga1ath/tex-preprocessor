#include <cstdlib>
#include "string"
#include "vector"
#include "map"
#include "set"

#include "basic_HM.h"


TypeVariable::TypeVariable() = default;


TypeOperator::TypeOperator (
    const std::string& name,
    const std::vector<TypeVariable*>& types
) {
    this->name = name;
    this->types = types;
}



bool is_number(const std::string& name) {
    char *p;
    strtol(name.c_str(), &p, 10);

    return !*p;
}


Node* get_base_type(Node* object) {
    auto* type_var = dynamic_cast<TypeVariable*>(object);

    if (type_var != nullptr) {
        if (type_var->instance != nullptr) {
            type_var->instance = get_base_type(type_var->instance);
            return type_var->instance;
        }
    }

    return object;
}


bool any_type_match(
    Node* target,
    const std::vector<TypeVariable*>& source
) {
    for (auto& type : source) {
        bool is_match = type_match(target, type);
        if (is_match) {
            return true;
        }
    }

    return false;
}


bool type_match(
    Node* target,
    Node* source
) {
    Node* source_base_type = get_base_type(source);

    if (source_base_type == target) {
        return true;
    } else {
        auto* type_op = dynamic_cast<TypeOperator*>(source_base_type);
        if (type_op != nullptr) {
            return any_type_match(target, type_op->types);
        }
        return false;
    }
}


bool is_generic_type(
    Node* target,
    const std::vector<TypeVariable *> &source
) {
    return !any_type_match(target, source);
}


void unification(
    Node* type1,
    Node* type2
) {
    Node* base_type_1 = get_base_type(type1);
    Node* base_type_2 = get_base_type(type2);

    auto* type_var1 = dynamic_cast<TypeVariable*>(base_type_1);
    auto* type_var2 = dynamic_cast<TypeVariable*>(base_type_1);
    auto* type_op1 = dynamic_cast<TypeOperator*>(base_type_1);
    auto* type_op2 = dynamic_cast<TypeOperator*>(base_type_1);

    if (type_var1 != nullptr) {
        if (type_var1 != base_type_2) {
            if (type_match(type_var1, base_type_2)) {
                throw std::invalid_argument(
                        "Cannot unify recursively: " + type_var1->toString() + ", " + base_type_2->toString()
                );
            }
            type_var1->instance = base_type_2;
        }
    } else {
        if (type_op1 != nullptr && type_var2 != nullptr) {
            unification(type_var2, type_op1);
        } else {
            if (type_op1 != nullptr && type_op2 != nullptr) {
                if (type_op1->name != type_op2->name || type_op1->types.size() != type_op2->types.size()) {
                    throw std::invalid_argument(
                            "type mismatch: " + type_op1->toString() + " != " + type_op2->toString()
                    );
                }

                for (int i = 0; i < std::min(type_op1->types.size(), type_op2->types.size()); ++i) {
                    unification(type_op1->types[i], type_op2->types[i]);
                }
            } else {
                throw std::invalid_argument(
                        "cannot unify: " + type1->toString() + " and " + type2->toString()
                );
            }
        }
    }
}


Node* copy_type_rec(
    Node* type,
    const std::vector<TypeVariable *> &non_generic,
    std::map<TypeVariable*, TypeVariable*> mapping
) {
    Node* base_type = get_base_type(type);

    auto* type_var = dynamic_cast<TypeVariable*>(base_type);
    auto* type_op = dynamic_cast<TypeOperator*>(base_type);

    if (type_var != nullptr) {
        if (is_generic_type(type_var, non_generic)) {

            if (mapping.count(type_var) == 0) {
                mapping.emplace(type_var, new TypeVariable());
            }

            return mapping[type_var];
        } else {
            return type_var;
        }
    } else {
        if (type_op != nullptr) {
            std::vector<TypeVariable*> new_types;

            for (auto& cur_type : type_op->types) {
                Node* copied_obj = copy_type_rec(cur_type, non_generic, mapping);
                auto* copied_type = dynamic_cast<TypeVariable*>(copied_obj);

                if (copied_type != nullptr) {
                    new_types.push_back(copied_type);
                } else {
                    throw std::invalid_argument("Not a variable: " + copied_obj->toString());
                }
            }

            return new TypeOperator(type_op->name, new_types);
        } else {
            throw std::invalid_argument("Not a variable or operator: " + base_type->toString());
        }
    }
}


Node* copy_type(
    Node* type,
    const std::vector<TypeVariable *> &non_generic
) {
    std::map<TypeVariable*, TypeVariable*> mapping;

    return copy_type_rec(type, non_generic, mapping);
}


auto global_idents = name_table();
auto global_funcs = name_table();
auto global_funcs_body = std::map<std::string, std::pair<Node*, std::vector<std::pair<std::string, Value>>>>();

std::pair<Value, std::vector<std::pair<std::string, Value>>> analyse(
    Node *node,
    bool inside_func_or_block,
    std::vector<std::pair<std::string, Value>> local_vars,
    bool is_usub
) {
    Tag& current_tag = node->get_tag();

    if (current_tag == Tag::NUMBER) {
        double val = std::stod(node->get_label());

        if (is_usub) {
            val = -val;
        }

        return {{val, Value::dimensionless}, local_vars};
    }

    if (current_tag == Tag::IDENT) {
        const auto& ident_name = node->get_label();

        bool founded = false;
        Value val;
        for (const auto& pair : local_vars) {
            if (pair.first == ident_name) {
                founded = true;
                val = pair.second;
                break;
            }
        }

        if (global_idents.count(ident_name) > 0) {
            return {global_idents[ident_name], local_vars};
        } else if (inside_func_or_block && founded) {
            return {val, local_vars};
        } else {
            throw std::invalid_argument("IDENT does not exists; node: " + node->toString());
        }
    }

    if (current_tag == Tag::FUNC) {
        if (global_funcs.count(node->get_label()) > 0) {
            const auto& func_args = global_funcs_body.find(node->get_label())->second.second;

            if (node->fields.size() != func_args.size()) {
                throw std::invalid_argument(
                        "FUNC has an incorrect amount of args: " +
                        std::to_string(node->fields.size()) +
                        " instead of: " +
                        std::to_string(func_args.size()) +
                        " in node: " +
                        node->toString()
                );
            }

            for (int i = 0; i < node->fields.size(); i++) {
                const auto& calculated = analyse(
                    node->fields[i],
                    inside_func_or_block,
                    local_vars,
                    is_usub
                ).first;

                const auto& expected = func_args[i].second;

                if (!(
                    expected._type == Value::UNDEFINED ||
                    (
                        calculated._type == expected._type
                        ||
                        (
                            (calculated._type == Value::DOUBLE || calculated._type == Value::INFERRED_DOUBLE) &&
                            (expected._type == Value::DOUBLE || expected._type == Value::INFERRED_DOUBLE)
                        )
                        ||
                        (
                            (calculated._type == Value::MATRIX || calculated._type == Value::INFERRED_MATRIX) &&
                            (expected._type == Value::MATRIX || expected._type == Value::INFERRED_MATRIX)
                        )
                    ) && Value::is_equal_dim(calculated, expected)
                )) {
                    throw std::invalid_argument(
                            "FUNC argument has an incorrect type (or different dimensions): " +
                            Value::type_string(calculated._type) +
                            " instead of: " +
                            Value::type_string(expected._type) +
                            " in node: " +
                            node->toString()
                    );
                }
            }

            return {global_funcs.find(node->get_label())->second, local_vars};
        } else {
            for (const auto& local_var : local_vars) {
                if (local_var.first == node->get_label()) {
                    return {local_var.second, local_vars};
                }
            }
        }

        throw std::invalid_argument("FUNC does not exists; node: " + node->toString());
    }

    if (current_tag == Tag::BEGINC) {
        for (auto option : node->fields) {
            if (option->cond != nullptr) {
                Tag cond_tag = option->cond->get_tag();

                auto left = analyse(
                    option->cond->left,
                    inside_func_or_block,
                    local_vars,
                    is_usub
                );
                auto right = analyse(
                    option->cond->right,
                    inside_func_or_block,
                    left.second,
                    is_usub
                );

                if (
                    left.first._type == Value::UNDEFINED &&
                    option->cond->left->get_tag() == Tag::IDENT &&
                    (right.first._type == Value::DOUBLE || right.first._type == Value::INFERRED_DOUBLE)
                ) {
                    left.first._type = Value::INFERRED_DOUBLE;
                    left.first._dimension = right.first.get_dimension();

                    const std::string& ident_name = option->cond->left->get_label();

                    if (global_idents.count(ident_name) > 0) {
                        global_idents[ident_name] = Value(0.0, right.first.get_dimension());
                        global_idents[ident_name]._type = Value::INFERRED_DOUBLE;
                    } else {
                        if (inside_func_or_block) {
                            for (auto& local_var : local_vars) {
                                if (local_var.first == ident_name) {
                                    local_var.second = Value(0.0, right.first.get_dimension());
                                    local_var.second._type = Value::INFERRED_DOUBLE;
                                }
                            }
                        }
                    }
                }

                if (
                    right.first._type == Value::UNDEFINED &&
                    option->cond->right->get_tag() == Tag::IDENT &&
                    (left.first._type == Value::DOUBLE || left.first._type == Value::INFERRED_DOUBLE)
                ) {
                    right.first._type = Value::INFERRED_DOUBLE;
                    right.first._dimension = left.first.get_dimension();

                    const std::string& ident_name = option->cond->right->get_label();

                    if (global_idents.count(ident_name) > 0) {
                        global_idents[ident_name] = Value(0.0, left.first.get_dimension());
                        global_idents[ident_name]._type = Value::INFERRED_DOUBLE;
                    } else {
                        if (inside_func_or_block) {
                            for (auto& local_var : local_vars) {
                                if (local_var.first == ident_name) {
                                    local_var.second = Value(0.0, left.first.get_dimension());
                                    local_var.second._type = Value::INFERRED_DOUBLE;
                                }
                            }
                        }
                    }
                }

                if (!(
                    left.first._type == right.first._type &&
                    left.first._type == Value::DOUBLE &&
                    Value::check_dimensions(left.first.get_dimension(), right.first.get_dimension())
                )) {
                    if (left.first._type == Value::UNDEFINED) {
                        throw std::invalid_argument(
                                "Undefined value: " +
                                to_string(left.first) +
                                " in node: " +
                                option->toString()
                        );
                    }

                    if (right.first._type == Value::UNDEFINED) {
                        throw std::invalid_argument(
                                "Undefined value: " +
                                to_string(right.first) +
                                " in node: " +
                                option->toString()
                        );
                    }

                    if (left.first._type == Value::INFERRED_DOUBLE) {
                        throw std::invalid_argument(
                                "Cannot compare using inferred double value: " +
                                to_string(left.first) +
                                " in node: " +
                                option->toString()
                        );
                    }

                    if (right.first._type == Value::INFERRED_DOUBLE) {
                        throw std::invalid_argument(
                                "Cannot compare using inferred double value: " +
                                to_string(right.first) +
                                " in node: " +
                                option->toString()
                        );
                    }

                    throw std::invalid_argument(
                            "Cannot compare non double (or with different dimension) value: " +
                            to_string(left.first) +
                            " and value: " +
                            to_string(right.first) +
                            " in node: " +
                            option->toString()
                    );
                }

                switch (cond_tag) {
                    case Tag::GT:
                        if (left.first.get_double() > right.first.get_double()) {
                            return analyse(option->right, inside_func_or_block, right.second, is_usub);
                        } else {
                            continue;
                        }
                    case Tag::GEQ:
                        if (left.first.get_double() >= right.first.get_double()) {
                            return analyse(option->right, inside_func_or_block, right.second, is_usub);
                        } else {
                            continue;
                        }
                    case Tag::LT:
                        if (left.first.get_double() < right.first.get_double()) {
                            return analyse(option->right, inside_func_or_block, right.second, is_usub);
                        } else {
                            continue;
                        }
                    case Tag::LEQ:
                        if (left.first.get_double() <= right.first.get_double()) {
                            return analyse(option->right, inside_func_or_block, right.second, is_usub);
                        } else {
                            continue;
                        }
                    case Tag::EQ:
                        if (left.first.get_double() == right.first.get_double()) {
                            return analyse(option->right, inside_func_or_block, right.second, is_usub);
                        } else {
                            continue;
                        }
                    case Tag::NEQ:
                        if (left.first.get_double() != right.first.get_double()) {
                            return analyse(option->right, inside_func_or_block, right.second, is_usub);
                        } else {
                            continue;
                        }
                    default:
                        throw std::invalid_argument(
                                "Cannot compare with wrong condition tag: " +
                                to_string(cond_tag) +
                                " value: " +
                                to_string(left.first) +
                                " and value: " +
                                to_string(right.first) +
                                " in node: " +
                                option->toString()
                        );
                }
            } else {
                return analyse(option->right, inside_func_or_block, local_vars, is_usub);
            }
        }
    }

    if (current_tag == Tag::UADD || current_tag == Tag::NOT || current_tag == Tag::LPAREN) {
        return analyse(node->right, inside_func_or_block, local_vars, is_usub);
    }

    if (current_tag == Tag::USUB) {
        return analyse(node->right, inside_func_or_block, local_vars, true);
    }

    if (
        current_tag == Tag::ADD ||
        current_tag == Tag::SUB ||
        current_tag == Tag::AND ||
        current_tag == Tag::OR ||
        current_tag == Tag::LT ||
        current_tag == Tag::LEQ ||
        current_tag == Tag::GT ||
        current_tag == Tag::GEQ ||
        (current_tag == Tag::EQ && node->right->get_tag() != Tag::PLACEHOLDER) ||
        current_tag == Tag::NEQ
    ) {
        auto left = analyse(node->left, inside_func_or_block, local_vars, is_usub);
        auto right = analyse(node->right, inside_func_or_block, left.second, is_usub);

        if (
            left.first._type == Value::UNDEFINED &&
            node->left->get_tag() == Tag::IDENT &&
            (right.first._type == Value::DOUBLE || right.first._type == Value::INFERRED_DOUBLE)
        ) {
            left.first._type = Value::INFERRED_DOUBLE;
            left.first._dimension = right.first.get_dimension();
            const std::string& ident_name = node->left->get_label();

            if (global_idents.count(ident_name) > 0) {
                global_idents[ident_name] = Value(0.0, right.first.get_dimension());
                global_idents[ident_name]._type = Value::INFERRED_DOUBLE;
            } else {
                if (inside_func_or_block) {
                    for (int i = 0; i < local_vars.size(); i++) {
                        if (local_vars[i].first == ident_name) {
                            local_vars[i].second = Value(0.0, right.first.get_dimension());
                            local_vars[i].second._type = Value::INFERRED_DOUBLE;
                        }
                    }
                }
            }
        }

        if (
            left.first._type == Value::UNDEFINED &&
            node->left->get_tag() == Tag::IDENT &&
            (right.first._type == Value::MATRIX || right.first._type == Value::INFERRED_MATRIX)
        ) {
            left.first._type = Value::INFERRED_MATRIX;
            left.first._dimension = right.first.get_dimension();
            const std::string& ident_name = node->left->get_label();

            if (global_idents.count(ident_name) > 0) {
                global_idents[ident_name] = Value(right.first.get_matrix());
            } else {
                if (inside_func_or_block) {
                    for (int i = 0; i < local_vars.size(); i++) {
                        if (local_vars[i].first == ident_name) {
                            local_vars[i].second = Value(right.first.get_matrix());
                        }
                    }
                }
            }
        }

        if (
            right.first._type == Value::UNDEFINED &&
            node->right->get_tag() == Tag::IDENT &&
            (left.first._type == Value::DOUBLE || left.first._type == Value::INFERRED_DOUBLE)
        ) {
            right.first._type = Value::INFERRED_DOUBLE;
            right.first._dimension = left.first.get_dimension();
            const std::string& ident_name = node->right->get_label();

            if (global_idents.count(ident_name) > 0) {
                global_idents[ident_name] = Value(0.0, left.first.get_dimension());
                global_idents[ident_name]._type = Value::INFERRED_DOUBLE;
            } else {
                if (inside_func_or_block) {
                    for (int i = 0; i < local_vars.size(); i++) {
                        if (local_vars[i].first == ident_name) {
                            local_vars[i].second = Value(0.0, left.first.get_dimension());
                            local_vars[i].second._type = Value::INFERRED_DOUBLE;
                        }
                    }
                }
            }
        }

        if (
            right.first._type == Value::UNDEFINED &&
            node->left->get_tag() == Tag::IDENT &&
            (left.first._type == Value::MATRIX || left.first._type == Value::INFERRED_MATRIX)
        ) {
            right.first._type = Value::INFERRED_MATRIX;
            right.first._dimension = left.first.get_dimension();
            const std::string& ident_name = node->left->get_label();

            if (global_idents.count(ident_name) > 0) {
                global_idents[ident_name] = Value(left.first.get_matrix());
            } else {
                if (inside_func_or_block) {
                    for (int i = 0; i < local_vars.size(); i++) {
                        if (local_vars[i].first == ident_name) {
                            local_vars[i].second = Value(left.first.get_matrix());
                        }
                    }
                }
            }
        }

        if (!(
            (left.first._type == Value::DOUBLE || left.first._type == Value::INFERRED_DOUBLE) &&
            (right.first._type == Value::DOUBLE || right.first._type == Value::INFERRED_DOUBLE) &&
            Value::check_dimensions(left.first.get_dimension(), right.first.get_dimension())
            ||
            (left.first._type == Value::MATRIX || left.first._type == Value::INFERRED_MATRIX) &&
            (right.first._type == Value::MATRIX || right.first._type == Value::INFERRED_MATRIX) &&
            Value::is_matrix_equals_dims(left.first.get_matrix(), right.first.get_matrix()) &&
            (current_tag == Tag::ADD || current_tag == Tag::SUB)
        )) {
            if (left.first._type == Value::UNDEFINED) {
                throw std::invalid_argument(
                        "Undefined value: " +
                        to_string(left.first) +
                        " in node: " +
                        node->toString()
                );
            }

            if (right.first._type == Value::UNDEFINED) {
                throw std::invalid_argument(
                        "Undefined value: " +
                        to_string(right.first) +
                        " in node: " +
                        node->toString()
                );
            }

            throw std::invalid_argument(
                    "Cannot ADD/SUB/AND/OR/LT/LEQ/GT/GEQ/EQ/NEQ non double (or with different dimension) value: " +
                    to_string(left.first) +
                    " and value: " +
                    to_string(right.first) +
                    " in node: " +
                    node->toString()
            );
        }

        return {right.first, local_vars};
    }

    if (current_tag == Tag::MUL || current_tag == Tag::DIV || current_tag == Tag::FRAC) {
        auto left = analyse(node->left, inside_func_or_block, local_vars, is_usub);
        auto right = analyse(node->right, inside_func_or_block, left.second, is_usub);

        if (
            left.first._type == Value::UNDEFINED &&
            node->left->get_tag() == Tag::IDENT &&
            (right.first._type == Value::DOUBLE || right.first._type == Value::INFERRED_DOUBLE)
        ) {
            left.first._type = Value::INFERRED_DOUBLE;
            const std::string& ident_name = node->left->get_label();

            if (global_idents.count(ident_name) > 0) {
                global_idents[ident_name] = Value(0.0);
                global_idents[ident_name]._type = Value::INFERRED_DOUBLE;
            } else {
                if (inside_func_or_block) {
                    for (auto& local_var : local_vars) {
                        if (local_var.first == ident_name) {
                            local_var.second = Value(0.0);
                            local_var.second._type = Value::INFERRED_DOUBLE;
                        }
                    }
                }
            }
        }

        if (
            left.first._type == Value::UNDEFINED &&
            node->left->get_tag() == Tag::IDENT &&
            (right.first._type == Value::MATRIX || right.first._type == Value::INFERRED_MATRIX)
        ) {
            left.first._type = Value::INFERRED_MATRIX;
            left.first._dimension = right.first.get_dimension();
            const std::string& ident_name = node->left->get_label();

            if (global_idents.count(ident_name) > 0) {
                global_idents[ident_name] = Value(right.first.get_matrix());
            } else {
                if (inside_func_or_block) {
                    for (int i = 0; i < local_vars.size(); i++) {
                        if (local_vars[i].first == ident_name) {
                            local_vars[i].second = Value(right.first.get_matrix());
                        }
                    }
                }
            }
        }

        if (
            right.first._type == Value::UNDEFINED &&
            node->right->get_tag() == Tag::IDENT &&
            (left.first._type == Value::DOUBLE || left.first._type == Value::INFERRED_DOUBLE)
        ) {
            right.first._type = Value::INFERRED_DOUBLE;
            const std::string& ident_name = node->right->get_label();

            if (global_idents.count(ident_name) > 0) {
                global_idents[ident_name] = Value(0.0);
                global_idents[ident_name]._type = Value::INFERRED_DOUBLE;
            } else {
                if (inside_func_or_block) {
                    for (auto& local_var : local_vars) {
                        if (local_var.first == ident_name) {
                            local_var.second = Value(0.0);
                            local_var.second._type = Value::INFERRED_DOUBLE;
                        }
                    }
                }
            }
        }

        if (
            right.first._type == Value::UNDEFINED &&
            node->left->get_tag() == Tag::IDENT &&
            (left.first._type == Value::MATRIX || left.first._type == Value::INFERRED_MATRIX)
        ) {
            right.first._type = Value::INFERRED_MATRIX;
            right.first._dimension = left.first.get_dimension();
            const std::string& ident_name = node->left->get_label();

            if (global_idents.count(ident_name) > 0) {
                global_idents[ident_name] = Value(left.first.get_matrix());
            } else {
                if (inside_func_or_block) {
                    for (int i = 0; i < local_vars.size(); i++) {
                        if (local_vars[i].first == ident_name) {
                            local_vars[i].second = Value(left.first.get_matrix());
                        }
                    }
                }
            }
        }

        if (!(
            (left.first._type == Value::DOUBLE || left.first._type == Value::INFERRED_DOUBLE) &&
            (right.first._type == Value::DOUBLE || right.first._type == Value::INFERRED_DOUBLE)
            ||
            current_tag == Tag::MUL &&
            (left.first._type == Value::MATRIX || left.first._type == Value::INFERRED_MATRIX) &&
            (right.first._type == Value::MATRIX || right.first._type == Value::INFERRED_MATRIX) &&
            (left.first.get_matrix()[0].size() == right.first.get_matrix().size())
        )) {
            if (left.first._type == Value::UNDEFINED) {
                throw std::invalid_argument(
                        "Undefined value: " +
                        to_string(left.first) +
                        " in node: " +
                        node->toString()
                );
            }

            if (right.first._type == Value::UNDEFINED) {
                throw std::invalid_argument(
                        "Undefined value: " +
                        to_string(right.first) +
                        " in node: " +
                        node->toString()
                );
            }

            throw std::invalid_argument(
                    "Cannot MUL/DIV/FRAC non double value: " +
                    to_string(left.first) +
                    " and value: " +
                    to_string(right.first) +
                    " in node: " +
                    node->toString()
            );
        }

        if (current_tag == Tag::MUL) {
            if (left.first._type == Value::MATRIX || left.first._type == Value::INFERRED_MATRIX) {
                return {
                    {
                        left.first.get_matrix(),
                        Value::sum_dimensions(left.first.get_dimension(), right.first.get_dimension())
                    },
                    right.second
                };
            } else {
                return {
                    {Value::sum_dimensions(left.first.get_dimension(), right.first.get_dimension())},
                    right.second
                };
            }
        } else {
            return {
                {Value::sub_dimensions(left.first.get_dimension(), right.first.get_dimension())},
                right.second
            };
        }
    }

    if (current_tag == Tag::POW) {
        auto left = analyse(node->left, inside_func_or_block, local_vars, is_usub);
        auto right = analyse(node->right, inside_func_or_block, left.second, is_usub);

        if (!(
            (left.first._type == Value::DOUBLE || left.first._type == Value::INFERRED_DOUBLE) &&
            (right.first._type == Value::DOUBLE || right.first._type == Value::INFERRED_DOUBLE) &&
            Value::is_dimensionless(right.first) &&
            right.first.get_double() == trunc(right.first.get_double()) &&
            right.first.get_double() >= 1
        )) {
            if (left.first._type == Value::UNDEFINED) {
                throw std::invalid_argument(
                        "Undefined value: " +
                        to_string(left.first) +
                        " in node: " +
                        node->toString()
                );
            }

            if (right.first._type == Value::UNDEFINED) {
                throw std::invalid_argument(
                        "Undefined value: " +
                        to_string(right.first) +
                        " in node: " +
                        node->toString()
                );
            }

            throw std::invalid_argument(
                    "Cannot POW non double (or to dimensional or non integer degree) value: " +
                    to_string(left.first) +
                    " and value: " +
                    to_string(right.first) +
                    " in node: " +
                    node->toString()
            );
        }

        return {
            {Value::mul_dimensions(left.first.get_dimension(), (int) right.first.get_double())},
            right.second
        };
    }

    if (current_tag == Tag::SUM || current_tag == Tag::PRODUCT) {
        auto left = analyse(node->left, inside_func_or_block, local_vars, is_usub);
        auto cond = analyse(node->cond, inside_func_or_block, left.second, is_usub);
        auto right = analyse(node->right, inside_func_or_block, cond.second, is_usub);

        if (!(
            (left.first._type == Value::DOUBLE || left.first._type == Value::INFERRED_DOUBLE) &&
            (right.first._type == Value::DOUBLE || right.first._type == Value::INFERRED_DOUBLE) &&
            Value::is_dimensionless(left.first) &&
            Value::is_dimensionless(cond.first)
        )) {
            if (left.first._type == Value::UNDEFINED) {
                throw std::invalid_argument(
                        "Undefined value: " +
                        to_string(left.first) +
                        " in node: " +
                        node->toString()
                );
            }

            if (right.first._type == Value::UNDEFINED) {
                throw std::invalid_argument(
                        "Undefined value: " +
                        to_string(right.first) +
                        " in node: " +
                        node->toString()
                );
            }

            throw std::invalid_argument(
                    "Cannot use SUM operator on non double (or on dimensional) value: " +
                    to_string(left.first) +
                    " and value: " +
                    to_string(cond.first) +
                    " in node: " +
                    node->toString()
            );
        }

        if (current_tag == Tag::SUM) {
            return analyse(node->right, inside_func_or_block, local_vars, is_usub);
        } else {
            return {
                Value::mul_dimensions(
                    right.first.get_dimension(),
                    floor(cond.first.get_double() - left.first.get_double())
                ),
                right.second
            };
        }
    }

    if (current_tag == Tag::DIMENSION) {
        return {
            {dimensions.find(node->get_label())->second},
            local_vars
        };
    }

    if (current_tag == Tag::ABS) {
        auto right = analyse(node->right, inside_func_or_block, local_vars, is_usub);

        if (right.first._type != Value::DOUBLE && right.first._type != Value::INFERRED_DOUBLE) {
            if (right.first._type == Value::UNDEFINED) {
                throw std::invalid_argument(
                        "Undefined value: " +
                        to_string(right.first) +
                        " in node: " +
                        node->toString()
                );
            }

            throw std::invalid_argument(
                    "Cannot use ABS operator on non double value: " +
                    to_string(right.first) +
                    " in node: " +
                    node->toString()
            );
        }

        return right;
    }

    if (current_tag == Tag::EQ) {
        if (node->right->get_tag() != Tag::PLACEHOLDER) {
            return analyse(node->right, inside_func_or_block, local_vars, is_usub);
        } else {
            return analyse(node->left, inside_func_or_block, local_vars, is_usub);
        }
    }

    if (current_tag == Tag::SET) {
        if (inside_func_or_block) {
            const std::string& ident_name = node->left->get_label();

//            if (global_idents.count(ident_name) > 0) {
//                throw std::invalid_argument(
//                        "Local ident with name: " +
//                        ident_name +
//                        " is already exists in global scope, node: " +
//                        node->toString()
//                );
//            }
//
//            for (const auto& cur : local_vars) {
//                if (cur.first == ident_name) {
//                    throw std::invalid_argument(
//                            "Local ident with name: " +
//                            ident_name +
//                            " is already exists in local scope, node: " +
//                            node->toString()
//                    );
//                }
//            }

            const auto& res = analyse(node->right, inside_func_or_block, local_vars, is_usub);

            for (int i = 0; i < local_vars.size(); i++) {
                if (local_vars[i].first == ident_name) {
                    local_vars[i].second = res.first;
                    return {res.first, local_vars};
                }
            }

            local_vars.emplace_back(ident_name, res.first);

            return {res.first, local_vars};
        } else {
            if (node->left->get_tag() == Tag::IDENT) {
                const std::string& ident_name = node->left->get_label();

//                if (global_idents.count(ident_name) > 0) {
//                    throw std::invalid_argument(
//                            "Global ident with name: " +
//                            ident_name +
//                            " is already exists, node: " +
//                            node->toString()
//                    );
//                }

                global_idents.emplace(
                    ident_name,
                    analyse(node->right, inside_func_or_block, local_vars, is_usub).first
                );

                return {
                    Value(),
                    local_vars
                };
            } else if (node->left->get_tag() == Tag::FUNC) {
                std::vector<std::pair<std::string, Value>> res;
                for (const auto field : node->left->fields) {
                    if (field->get_tag() != Tag::IDENT) {
                        throw std::invalid_argument(
                                "FUNC arg is not an IDENT: " +
                                field->toString() +
                                " , node: " +
                                node->toString()
                        );
                    }

                    for (const auto& arg : res) {
                        if (arg.first == field->get_label()) {
                            throw std::invalid_argument(
                                    "FUNC arg is already exists: " +
                                    field->toString() +
                                    " , node: " +
                                    node->toString()
                            );
                        }
                    }

                    res.emplace_back(field->get_label(), Value());
                }

                const auto& to_return = analyse(
                    node->right,
                    true,
                    res,
                    is_usub
                );

                global_funcs.emplace(node->left->get_label(), to_return.first);

                global_funcs_body.emplace(
                        node->left->get_label(),
                        std::pair<Node*, std::vector<std::pair<std::string, Value>>>(
                            node->right,
                            std::vector<std::pair<std::string, Value>>(
                                    to_return.second.begin(),
                                    to_return.second.begin() + (long) res.size()
                            )
                        )
                );

                return to_return;
            } else {
                throw std::invalid_argument("Cannot analyse SET statement: " + node->toString());
            }
        }
    }

    if (current_tag == Tag::BEGINB) {
        for (int i = 0; i < node->fields.size(); ++i) {
            if (i == node->fields.size() - 1) {
                return analyse(node->fields[i], true, local_vars, is_usub);
            } else {
                const auto& res = analyse(
                    node->fields[i],
                    true,
                    local_vars,
                    is_usub
                );
                local_vars = res.second;
            }
        }
    }

    if (current_tag == Tag::BEGINM) {
        Matrix m;
        Value x;

        if (!node->fields.empty() && !node->fields[0]->fields.empty()) {
            x = analyse(node->fields[0]->fields[0], inside_func_or_block, local_vars, is_usub).first;
        }

        for (const auto& field : node->fields) {
            std::vector<Value> v;
            for (const auto& jt : field->fields) {
                v.push_back(x);
            }
            m.push_back(v);
        }
        return {Value(m), local_vars};
    }

    if (current_tag == Tag::WHILE) {
        analyse(node->cond, inside_func_or_block, local_vars, is_usub);
        return analyse(node->right, inside_func_or_block, local_vars, is_usub);
    }

    if (
        current_tag == Tag::PLACEHOLDER ||
        current_tag == Tag::KEYWORD ||
        current_tag == Tag::CEIL ||
        current_tag == Tag::FLOOR
    ) {
        auto to_return = Value();
        to_return._type = Value::INFERRED_DOUBLE;

        return {to_return, local_vars};
    }

    if (
        current_tag == Tag::GRAPHIC ||
        current_tag == Tag::RANGE ||
        current_tag == Tag::LIST
    ) {
        return {Value(), local_vars};
    }

    if (current_tag == Tag::IF) {
        const auto& res = analyse(node->cond, inside_func_or_block, local_vars, is_usub);
        return analyse(node->right, inside_func_or_block, res.second, is_usub);
    }

    if (current_tag == Tag::TRANSP) {
        const auto& res = analyse(node->left, inside_func_or_block, local_vars, is_usub);

        return {
            Value::transpose(res.first),
            res.second
        };
    }

    throw std::invalid_argument("Cannot analyse node: " + node->toString());
}
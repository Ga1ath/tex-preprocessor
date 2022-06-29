#pragma once

#include "Coordinate.h"


class Value;

class Node;

typedef struct Parser {
	std::vector<Token> program;
	int i = 0;

	Token *next();

	Token *cur();

	Token *get();

	bool skip(Tag);

	void init(std::vector<Token> &);

	Node *unexpr(Token *);

	Node *binexpr(Token *, Node *);

	Node *expression(int = 0);

	std::vector<Node *> block(Tag = NONE);

	std::vector<Node *> line();

	std::vector<Node *> cases();

	std::vector<Node *> list(Tag close);

	std::vector<Node *> matrix();

	Node * arg(Tag open);

	void wait(Tag stop);
} Parser;


typedef std::map<std::string, Value> name_table;


struct Replacement;


typedef std::map<Coordinate, Replacement> replacement_map;


class Node {
	friend struct Parser;

	Coordinate _coord;
	Tag _tag = ERROR;
	std::string _label;
	int _priority = 0;
public:
	static name_table global;
	static replacement_map reps;
	Node *left = nullptr;
	Node *right = nullptr;
	Node *cond = nullptr;
	std::vector<Node *> fields;

	Node();

	Node(const Node &n);

	Node(Token *t);

	static void save_rep(const Coordinate&, Tag, size_t, size_t);

	virtual ~Node();

	void print(const std::string& pref) const;

	void set_tag(Tag t);

    Tag& get_tag();

    std::string& get_label();

    std::string& toString();

	Value exec(name_table *nt);

	static void copy_defs(name_table &local, name_table *ptr);

	static Value &lookup(const std::string& name, name_table *ptr, const Coordinate&);

	static void def(const std::string& name, const Value&, name_table *ptr);

    void semantic_analysis();
};
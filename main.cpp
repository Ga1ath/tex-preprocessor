#include <iostream>
#include "Defines.h"
#include "Coordinate.h"
#include "Error.h"
#include "FileHandler.h"
#include "Lexer.h"
#include "Node.h"
#include "Value.h"

ProgramString Position::ps;
name_table Node::global;
replacement_map Node::reps;

std::string make_replacement(std::string prog, replacement_map m) {
	std::string res = "";
	size_t index = 0;
	for (auto it = m.begin(); it != m.end(); ++it) {
		res += prog.substr(index, (*it).second.begin - index);
		if ((*it).second.tag == GRAPHIC) {
			res += "{" + to_plot((*it).second.replacement) + "}";
		}
		else {
			res += "{" + to_string((*it).second.replacement) + "}";
		}
		index = (*it).second.end;
	}
	res += prog.substr(index);
	return res;
}

int main(int argc, char *argv[]) {
	Parser B;

	bool ok = true;
	bool replace = false;   //файл не будет перезаписан по-умолчанию

	const char *file_in;
	const char *file_out;

	if (argc < 2 || argc > 3) { //число аргументов должно быть равно 1 или 2
		file_in = "test.tex";
		file_out = "_test.tex";
		replace = true;
		//std::cerr << "Usage: " << argv[0] << " input [output]" << std::endl;
		//return 1;
	}
	else {
		file_in = argv[1];
		if (argc == 2 ||
			!std::strcmp(file_in, argv[2])) { //если указан один аргумент или 1 и 2 аргументы совпадают
			file_out = new char[std::strlen(file_in) + 2];        //то файл будет перезаписан
			std::strcpy(const_cast<char *>(file_out), "_");
			std::strcat(const_cast<char *>(file_out), file_in);
			replace = true;
		}
		else { file_out = argv[2]; }
	}
	FileHandler &fh = FileHandler::Instance(file_in, file_out);
	if (!fh.good()) {
		std::cerr << file_in << ":" << "Failed to initialize" << std::endl;
		ok = false;
	}

	Node *res = nullptr;
	while (ok) {
		Position::ps = fh.next();
		if (Position::ps.program.empty()) { break; }
		Lexer l;
		try {
			std::vector<Token> p = l.program_to_tokens(Position::ps);
			for (size_t i = 0; i < p.size(); ++i) printf("%s\n", to_string(p[i]).c_str());
			B.init(p);
			res = new Node();
			res->fields = B.block(NONE);
			res->set_tag(ROOT);
			res->print("");
			res->exec();

			//std::string replacement = Position::ps.program;
			std::string replacement = make_replacement(Position::ps.program, Node::reps);
			fh.print_to_out(replacement);
			Node::reps.clear();
		}
		catch (Error err) {
			std::cerr << file_in << ":" << err.what() << std::endl;
			ok = false;
		}
		catch (Value::BadType err) {
			std::cerr << file_in << ":" << err.what() << std::endl;
			ok = false;
		}
		catch (std::exception err) {
			std::cerr << file_in << ":" << err.what() << std::endl;
			ok = false;
		}
		if (res) delete res;
	}
	if (ok) {                   //если удалось обработать файл и
		if (replace) {          //если надо перезаписать файл
			fh.replace_files();
		}
	}
	else { //если не удалось обработать файл, то удалить выходной файл
		fh.remove_out();
	}
	//if (replace) {
	//    delete[] file_out;
	//}

#ifdef _MSC_VER
	getchar();
#endif
	return 0;
}
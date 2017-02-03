#pragma once

#include <fstream>
#include <cstring>

class FileHandler {
public:
	static FileHandler& Instance(const char *fin, const char *fout)
	{
		static FileHandler fh(fin, fout);
		return fh;
	}
	ProgramString next();   //найти следующее окружение preproc
	void print_to_out(std::string r) { out_ << r; } //печать в выходной файл
	int replace_files() {   //замена исходного файла выходным
		close();
		if (!std::remove(fin_)) {
			if (std::rename(fout_, fin_)) std::cerr << "Couldn't rename file: " << fout_ << " to " << fin_ << std::endl;
			else return 0;
		}
		else std::cerr << "Couldn't remove file: " << fin_ << std::endl;
		return 1;
	}
	int remove_out() {      //удаление выходного файла
		close();
		if (std::remove(fout_)) {
			std::cerr << "Couldn't remove file: " << fin_ << std::endl;
			return 1;
		}
		return 0;
	}
	bool good() {
		return in_.good() && out_.good();
	}
private:
	static const char *begin_;
	static const char *end_;
	const char *fin_;
	const char *fout_;
	std::ifstream in_;
	std::ofstream out_;
	size_t line_;

	void close() {
		if (in_.is_open()) in_.close();
		if (out_.is_open()) out_.close();
	}
	FileHandler(const char *fin, const char *fout) : line_(0), fin_(fin), fout_(fout)
	{
		in_.open(fin_);
		out_.open(fout_);
	}
	~FileHandler() { close(); }
	FileHandler(FileHandler const&) = delete;
	FileHandler& operator=(FileHandler const&) = delete;
};

const char *FileHandler::begin_ = "\\begin{preproc}";
const char *FileHandler::end_ = "\\end{preproc}";

ProgramString FileHandler::next() {

	std::string tmp = "";

	std::string program = "";
	Coordinate c_begin(line_);
	Coordinate c_end(line_);

	ProgramString ps;

	while (std::getline(in_, tmp)) {
		++line_;
		size_t comment = tmp.find("%");
		size_t res = tmp.find(begin_);
		//в строке есть подстрока \begin{preproc} и она находится до %, если % есть
		if (res != std::string::npos && res < comment) {
			c_begin = Coordinate{ line_, res + std::strlen(begin_) + 1 };
			program += tmp + "\n";

			res = tmp.find(end_);    //если \end{preproc} на той же строке
			if (res != std::string::npos && res < comment) {
				c_end = Coordinate{ line_, res + 1 };
			}
			else {
				while (std::getline(in_, tmp)) {
					++line_;
					comment = tmp.find("%");
					res = tmp.find(end_);
					program += tmp + "\n";
					if (res != std::string::npos && res < comment) {
						c_end = Coordinate{ line_, res + 1 };
						break;
					}
				}
			}
			break;
		}
		//строки вне \begin_{preproc}...\end_{preproc} можно сразу писать в файл
		out_ << tmp << std::endl;
	}

	ps.program = program;
	ps.begin = c_begin;
	ps.end = c_end;
	ps.length = program.length();

	return ps;
}

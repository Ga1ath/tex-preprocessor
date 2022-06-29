#pragma once

#include <fstream>
#include <cstring>

#include "Coordinate.h"


class FileHandler {
public:
    static FileHandler& Instance(const char *fin, const char *fout) {
        static FileHandler fh(fin, fout);
        return fh;
    }

	ProgramString next();   //найти следующее окружение preproc

    void print_to_out(const std::string& r);

    int replace_files();

	int remove_out();

	bool good();

    FileHandler(FileHandler const&) = delete;
    FileHandler& operator=(FileHandler const&) = delete;

private:
	static const char *begin_;
	static const char *end_;
	const char *fin_;
	const char *fout_;
	std::ifstream in_;
	std::ofstream out_;
	size_t line_;

	void close();

	FileHandler(const char *fin, const char *fout);

	~FileHandler();
};


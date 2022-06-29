#include <fstream>
#include <cstring>

#include "FileHandler.h"


void FileHandler::print_to_out(const std::string& r) {
    out_ << r; //печать в выходной файл
}

int FileHandler::replace_files() {   //замена исходного файла выходным
    close();
    if (!std::remove(fin_)) {
        if (std::rename(fout_, fin_)) std::cerr << "Couldn't rename file: " << fout_ << " to " << fin_ << std::endl;
        else return 0;
    }
    else std::cerr << "Couldn't remove file: " << fin_ << std::endl;
    return 1;
}

int FileHandler::remove_out() {      //удаление выходного файла
    close();
    if (std::remove(fout_)) {
        std::cerr << "Couldn't remove file: " << fin_ << std::endl;
        return 1;
    }
    return 0;
}

bool FileHandler::good() {
    return in_.good() && out_.good();
}

void FileHandler::close() {
    if (in_.is_open()) in_.close();
    if (out_.is_open()) out_.close();
}

FileHandler::FileHandler(const char *fin, const char *fout) : line_(0), fin_(fin), fout_(fout) {
    in_.open(fin_);
    out_.open(fout_);
}

FileHandler::~FileHandler() {
    close();
}

const char *FileHandler::begin_ = "\\begin{preproc}";
const char *FileHandler::end_ = "\\end{preproc}";

ProgramString FileHandler::next() {
    std::string tmp;
    std::string program;
    Coordinate c_begin(line_);
    Coordinate c_end(line_);
    ProgramString ps;

    while (std::getline(in_, tmp)) {
        ++line_;
        size_t comment = tmp.find('%');
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
                    comment = tmp.find('%');
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

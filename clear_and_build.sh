rm -r "$(pwd)"/cmake-build-debug
mkdir "$(pwd)"/cmake-build-debug
cd "$(pwd)"/cmake-build-debug || exit
cmake ..
cmake -build .
make

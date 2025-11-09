set -ex

cmake . -B build
cd build
make

./cand

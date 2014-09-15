mkdir -p test
./CBSE.py def.yaml -o test --no-include-helper
clang++ -std=c++11 -c test/Components.cpp -o /dev/null

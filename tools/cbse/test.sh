mkdir -p test
./CBSE.py def.yaml -o test
cp test/components/skel/* test/components/
c++ --std=c++11 -c test/CBSEBackend.cpp -o /dev/null

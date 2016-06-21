mkdir -p test
./CBSE.py -s def.yaml -o test
cp test/components/skeletons/* test/components/
c++ --std=c++11 -c test/backend/CBSEBackend.cpp -o /dev/null

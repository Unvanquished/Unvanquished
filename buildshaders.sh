#!/bin/sh

exec > daemon/src/engine/renderer/shaders.cpp

cd main

cat <<EOF
#include <string>
#include <unordered_map>
EOF

for f in glsl/*.glsl
do
    n=$(basename $f .glsl)
    echo 'const char '$n'[] = {'
    ( cat $f; perl -e 'printf "%c", 0;' ) | xxd -i
    echo '};'
done

cat <<EOF
std::unordered_map<std::string, const char *> shadermap( {
EOF

for f in glsl/*.glsl
do
    n=$(basename $f .glsl)
    echo '{ "'$f'", '$n'},'
done
echo '} );'

exit 0

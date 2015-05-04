#!/bin/sh

exec > src/engine/renderer/shaders.cpp

cat <<EOF
#include <string>
#include <unordered_map>

std::unordered_map<std::string, std::string> shadermap( {
EOF

cd main
for f in glsl/*.glsl
do
    echo '{ "'$f'", R"('
    perl -p0e 's%/(\*.*?\*/|/[^\n]*)%%sg' $f
    echo ')" },'
done
echo '} );'

exit 0

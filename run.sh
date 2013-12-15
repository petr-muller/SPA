LLVM_LEVEL=../../../..

clang-check ~/Desktop/bc/test.c -ast-dump --

echo

${LLVM_LEVEL}/Release+Asserts/bin/clang -std=c11 -Wall -W -pedantic -g -Xclang -load -Xclang ${LLVM_LEVEL}/Release+Asserts/lib/libSPA.so -Xclang -add-plugin -Xclang SPA ~/Desktop/bc/test.c -o TEST;

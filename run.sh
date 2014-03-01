file=test1.c
if [[ ! -z $1 ]] ; then
    file=$1
fi

echo $file

~/bc/new/build/Release+Asserts/bin/clang -std=c11 -Wall -W -pedantic -g -Xclang -load -Xclang ~/bc/new/build/Release+Asserts/lib/libSPA.so -Xclang -add-plugin -Xclang SPA ~/bc/new/llvm/tools/clang/tools/SPA/examples/$file -o TEST;

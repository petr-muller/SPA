file=test1.c
if [[ ! -z $1 ]] ; then
    file=$1
fi

#echo $file
#echo ------------
#cat ~/bc/new/llvm/tools/clang/tools/SPA/examples/$file.c
#echo ------------

#make constraints
constraints=$(~/bc/new/build/Release+Asserts/bin/clang -std=c11 -Wall -W -pedantic -g -Xclang -load -Xclang ~/bc/new/build/Release+Asserts/lib/libSPA.so -Xclang -add-plugin -Xclang SPA ~/bc/new/llvm/tools/clang/tools/SPA/examples/$file.c -o TEST)

#create the LLVM IR
cd examples
clang -g3 -gcolumn-info -emit-llvm -c -o $file.bc $file.c -O0

#alias analysis
alias=$(opt -disable-output -basicaa --aa-eval -print-all-alias-modref-info $file.bc 2>&1 | grep -e 'MustAlias' -e 'MayAlias' | awk '{print $3 " " $5}' | sed 's/[%,]//g')

echo Aliases:
echo "$alias"
echo
echo Constraints:
echo "$constraints"

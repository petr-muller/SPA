file=test1.c
if [[ ! -z $1 ]] ; then
    file=$1
fi

#make constraints
constraints=$(~/bc/new/build/Release+Asserts/bin/clang -std=c11 -Wall -W -pedantic -g -Xclang -load -Xclang ~/bc/new/build/Release+Asserts/lib/libSPA.so -Xclang -add-plugin -Xclang SPA ~/bc/new/llvm/tools/clang/tools/SPA/examples/$file.c -o TEST)

#create the LLVM IR
cd examples
clang -g3 -gcolumn-info -emit-llvm -c -o $file.bc $file.c -O0

#alias analysis
aliases=$(opt -disable-output -basicaa --aa-eval -print-all-alias-modref-info $file.bc 2>&1 | grep -e 'MustAlias' -e 'MayAlias' | awk '{print $3 " " $5}' | sed 's/[%,]//g')

#llvm ir with debug info
llvmir=$(llvm-dis $file.bc -o - | grep -e ' !dbg !' -e '![0-9]\+ = metadata !{')

echo LLVM IR:
echo "$llvmir"
echo
echo Aliases:
echo "$aliases"
echo
echo Constraints:
echo "$constraints"
echo
echo Translated aliases:

res=''

while read alias; do
    var1=$(echo $alias | awk '{print $1}')
    var2=$(echo $alias | awk '{print $2}')
    if echo $var1 | grep '[0-9]\+' &>/dev/null; then dbg1=$(echo "$llvmir" | grep "[[:space:]]*%$var1 = .* !dbg ![0-9]\+"); dbg1=$(echo $dbg1 | awk '{print $NF}' | sed 's/!//g'); else dbg1='any'; fi
    if echo $var2 | grep '[0-9]\+' &>/dev/null; then dbg2=$(echo "$llvmir" | grep "[[:space:]]*%$var2 = .* !dbg ![0-9]\+"); dbg2=$(echo $dbg2 | awk '{print $NF}' | sed 's/!//g'); else dbg2='any'; fi
    if [ $dbg1 = 'any' -o $dbg2 = 'any' -o $dbg1 = $dbg2 ]; then
        if [ $dbg1 = 'any' ]; then
            dbg=$dbg2;
        else
            dbg=$dbg1;
        fi
        res="$res$(echo "$llvmir" | grep "^!$dbg = metadata !{.*}" | awk '{print $5 " " $7}' | sed 's/,//g' | tr '\n' ' ')"
        if [ $dbg1 = 'any' ]; then
            res="$res$(printf "$var1 ")"
        else
            res="$res$(printf "%s" "$(echo "$llvmir" | grep "[[:space:]]*%$var1 = load [[:alnum:]]*\*\+ %[[:alnum:]]\+, ")" | awk '{print $4 $5}' | grep -o '\*\+.*' | sed 's/[%,]//g' | sed 's/^\*//g' | tr '\n' ' ')"
        fi

        if [ $dbg2 = 'any' ]; then
            res="$res$(echo "$var2")"
        else
            res="$res$(printf "%s" "$(echo "$llvmir" | grep "[[:space:]]*%$var2 = load [[:alnum:]]*\*\+ %[[:alnum:]]\+, ")" | awk '{print $4 $5}' | grep -o '\*\+.*' | sed 's/[%,]//g' | sed 's/^\*//g')"
        fi
        res="$res"$'\n'
    fi
done <<< "$aliases"

printf "%s" "$res"

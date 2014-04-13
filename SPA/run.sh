#!/usr/bin/env bash

# Lukas Hellebrandt <xhelle04@fit.vutbr.cz>

# This script is highly implementation- and version- dependant.
# Its purpose is to get the set of constraints, set of aliases and check whether some of the constraints are violated. In that case, it found an undefined behavior.


function alias_with_func_name {
  while read row; do
    echo "$row" | grep -q '^Function: [_[:alnum:]]\+:.*'
    if [ $? -eq 0 ]; then
      curr_func=$(echo "$row" | awk '{print $2}' | sed 's/://')
    else
      echo "$curr_func $row"
    fi
  done
}

function get_function_names {
  while read row; do
    echo "$row" | grep -q '^define [^[:space:]]\+ @[_[:alnum:]]\+(.\+' || continue;
    name=$(echo "$row" | awk '{print $3}' | grep -o '@[_[:alnum:]]\+(' | sed -e 's/@//g' -e 's/(//g')
    present=1
    for tmp_name in $names; do
      if [ "$tmp_name" = "$name" ]; then
        present=0
      fi
    done
    if [ $present -eq 1 ]; then
      names="$names $name"
    fi
  done
  echo $names
}

function get_function_llvmir {
  input=$(cat -)
  ret=$(echo "$input" | tail -n +$(echo "$input" | grep -n "^define [^[:space:]]\+ @$1(.\+" | cut -d':' -f1))
  state=0 #no function matched
  while IFS= read row; do
    echo "$row" | grep -q '^define [^[:space:]]\+ @[^[:space:]]\+(.\+'
    if [ $? -eq 0 ]; then
      if [ $state -eq 0 ]; then
        tmp="$tmp"$'\n'"$row"
        state=1 #first function header printed
      else
        state=2 #skip all the following functions
      fi
    fi

    echo "$row" | grep -q '^[[:space:]]'
    if [ $? -eq 0 ]; then
      if [ $state -eq 1 ]; then
        tmp="$tmp"$'\n'"$row"
      fi
    fi

    echo "$row" | grep -q '^![0-9]\+ = '
    if [ $? -eq 0 ]; then
      tmp="$tmp"$'\n'"$row"
    fi
  done <<< "$ret"
  ret="$tmp"
  echo "$ret"
}

file=llvm/tools/clang/tools/SPA/examples/test2.c
if [[ ! -z $1 ]] ; then
    file=$1
fi

#strip the extension off the filename
file=${file%.*}

#make constraints
constraints=$(build/Release+Asserts/bin/clang -std=c11 -Wall -W -pedantic -g -Xclang -load -Xclang build/Release+Asserts/lib/libSPA.so -Xclang -add-plugin -Xclang SPA ${file}.c -o TEST)

#create the LLVM IR
build/Release+Asserts/bin/clang -g3 -gcolumn-info -emit-llvm -c -o ${file}.bc ${file}.c -O0 2>/dev/null

#alias analysis
aliases=$(build/Release+Asserts/bin/opt -disable-output -basicaa --aa-eval -print-all-alias-modref-info ${file}.bc 2>&1 | alias_with_func_name | grep -e 'MustAlias' -e 'MayAlias' | awk '{print $1 " " $4 " " $6}' | sed 's/[%,]//g')

#llvm ir with debug info
llvmir=$(build/Release+Asserts/bin/llvm-dis ${file}.bc -o - | grep -e '^define [^[:space:]]\+ @[_[:alnum:]]\+(.*' -e ' !dbg !' -e '![0-9]\+ = metadata !{')

function_names=$(echo "$llvmir" | get_function_names)

for function_name in $function_names; do #translate for each function separately
  function_aliases=$(echo "$aliases" | grep "^$function_name")
  function_llvmir=$(echo "$llvmir" | get_function_llvmir $function_name)

  [ $(echo "$function_aliases" | wc -l) -lt 2 ] && echo "$function_aliases" | grep -q '^[[:space:]]*$' && continue # if there are no aliases, continue

  #translate the aliases - map them to the original source code
  while read alias; do
      var1=$(echo $alias | awk '{print $2}')
      var2=$(echo $alias | awk '{print $3}')
      if echo $var1 | grep '^[0-9]\+$' &>/dev/null; then dbg1=$(echo "$function_llvmir" | grep "[[:space:]]*%$var1 = .* !dbg ![0-9]\+"); dbg1=$(echo $dbg1 | awk '{print $NF}' | sed 's/!//g'); else dbg1='any'; fi
      if echo $var2 | grep '^[0-9]\+$' &>/dev/null; then dbg2=$(echo "$function_llvmir" | grep "[[:space:]]*%$var2 = .* !dbg ![0-9]\+"); dbg2=$(echo $dbg2 | awk '{print $NF}' | sed 's/!//g'); else dbg2='any'; fi
      if [ "$dbg1" = 'any' -o "$dbg2" = 'any' -o "$dbg1" = "$dbg2" -o 1 -eq 1 ]; then #FIXME 1==1
          translated="$translated$function_name " # function name

          if [ "$dbg1" = 'any' ]; then # position of var1
              translated="${translated}any any "
          else
              translated="$translated$(echo "$function_llvmir" | grep "^!$dbg1 = metadata !{.*}" | awk '{print $5 " " $7}' | sed 's/,//g' | tr '\n' ' ')"
          fi
          if [ "$dbg2" = 'any' ]; then # position of var2
              translated="${translated}any any "
          else
              translated="$translated$(echo "$function_llvmir" | grep "^!$dbg2 = metadata !{.*}" | awk '{print $5 " " $7}' | sed 's/,//g' | tr '\n' ' ')"
          fi

          if [ "$dbg1" = 'any' ]; then # var1
              translated="$translated$(printf "$var1 ")"
          else
              translated="$translated$(printf "%s" "$(echo "$function_llvmir" | grep "[[:space:]]*%$var1 = load [_[:alnum:]]*\*\+ %[_[:alpha:]][_[:alnum:]]*")" | awk '{print $4 $5}' | grep -o '\*\+.*' | sed 's/[%,]//g' | sed 's/^\*//g' | tr '\n' ' ')"
          fi
          if [ "$dbg2" = 'any' ]; then # var2
              translated="$translated$(printf "%s" "$var2")"
          else
              translated="$translated$(printf "%s" "$(echo "$function_llvmir" | grep "[[:space:]]*%$var2 = load [_[:alnum:]]*\*\+ %[_[:alpha:]][_[:alnum:]]*")" | awk '{print $4 $5}' | grep -o '\*\+.*' | sed 's/[%,]//g' | sed 's/^\*//g')"
          fi
          translated="$translated"$'\n'
      fi
  done <<< "$function_aliases"
done
translated="${translated%?}"

#<<DEBUG
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
echo "$translated"
echo
echo Results:
#DEBUG

for function_name in $function_names; do
  while read constraint; do
      if [ $(echo $constraint | wc -w) -eq 6 ]; then
          echo "Possible undefined behavior at [$(echo $constraint | awk '{print $2 "," $3}')] - variable \"$(echo $constraint | awk '{print $6}')\""
      else
          if [ $(echo $constraint | wc -w) -lt 3 ]; then
              break
          fi
          while read Alias; do
              
              if [ "$(echo "$Alias" | awk '{print $6 $7}')" = "$(echo "$constraint" | awk '{print $6 $7}')" -o "$(echo "$Alias" | awk '{print $6 $7}')" = "$(echo "$constraint" | awk '{print $7 $6}')" ]; then
                  echo "Possible undefined behavior at [$(echo $constraint | awk '{print $2 "," $3}')] - \"$(echo $constraint | awk '{print $6}')\" aliases with \"$(echo $constraint | awk '{print $7}')\""
              fi
          done <<< "$(echo "$translated" | grep "^$function_name ")"
      fi
  done <<< "$(echo "$constraints" | grep "^$function_name ")"
done

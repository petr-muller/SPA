all: build run.sh debugrun.sh
	cd build; \
	make -j 8

build: llvm llvm/tools/clang/tools/SPA
	mkdir -p build
	cd build; \
	../llvm/configure --enable-optimized

debugrun.sh: run.sh
	sed -e 's/^<<DEBUG/#<<DEBUG/g' -e 's/^DEBUG/#DEBUG/g' run.sh > debugrun.sh

run.sh: SPA/run.sh
	cp SPA/run.sh .

llvm/tools/clang/tools/SPA: llvm SPA
	cp -r SPA llvm/tools/clang/tools
	cp SPA/top-level-makefile/Makefile llvm/tools/clang/tools

llvm:
	svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm;
	cd llvm/tools; \
	svn co http://llvm.org/svn/llvm-project/cfe/trunk clang; \
	cd ../projects; \
	svn co http://llvm.org/svn/llvm-project/compiler-rt/trunk compiler-rt ; \

.PHONY: clean remove run debugrun

clean:
	rm -rf build

remove:
	rm -rf build llvm

run: run.sh
	./run.sh

debugrun: debugrun.sh
	./debugrun.sh

# \/ temporary - shall be deleted \/
constraints:
	build/Release+Asserts/bin/clang -c -std=c11 -Wall -W -pedantic -g -Xclang -load -Xclang build/Release+Asserts/lib/libSPA.so -Xclang -add-plugin -Xclang SPA SPA/examples/function.c -o TEST
ast:
	build/Release+Asserts/bin/clang -std=c11 -Xclang -ast-dump -fsyntax-only SPA/examples/array3N.c

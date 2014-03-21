all: $(filter-out $(wildcard build), build)
	cd build; \
	make -j 8

build: llvm llvm/tools/clang/tools/SPA
	mkdir -p build
	cd build; \
	../llvm/configure --enable-optimized

llvm/tools/clang/tools/SPA: llvm SPA
	cp -r SPA llvm/tools/clang/tools
	cp SPA/top-level-makefile/Makefile llvm/tools/clang/tools

llvm:
	svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm;
	cd llvm/tools; \
	svn co http://llvm.org/svn/llvm-project/cfe/trunk clang; \
	cd projects; \
	svn co http://llvm.org/svn/llvm-project/compiler-rt/trunk compiler-rt ; \
	cd ../..

clean:
	rm -rf build

remove:
	rm -rf build llvm

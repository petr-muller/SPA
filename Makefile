all: build run.sh
	cd build; \
	make -j 8

build: llvm llvm/tools/clang/tools/SPA
	mkdir -p build
	cd build; \
	../llvm/configure --enable-optimized

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

.PHONY: all
all: build

build/Makefile: CMakeLists.txt
	mkdir -p build
	(cd build; cmake ..);

.PHONY: build
build: build/Makefile
	$(MAKE) -C build

.PHONY: clean
clean:
	rm -r build/

.PHONY: run
run: build
	./build/test_cxx

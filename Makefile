TARGETS=release debug sim_release sim_debug

all: $(TARGETS)
	cp build/release/*.rpm .

$(TARGETS):
	$(MAKE) -j16 -C build/$@
	$(MAKE) -j16 -C build/$@ package

doc:
	$(MAKE) -C build/release/ doc
	$(MAKE) -C build/release/doxygen/latex/
	cp build/release/doxygen/latex/refman.pdf manual.pdf
	rm -rf build/release/doxygen/latex/
	tar -cf manual.tar build/release/doxygen/
	gzip manual.tar

clean:
	$(RM) -r build

count:
	wc -l `find . -iname '*.c' && find . -iname '*.hh' && find . -iname '*.cpp'`

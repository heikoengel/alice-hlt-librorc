TARGETS=release debug sim_release sim_debug

all: $(TARGETS)

$(TARGETS):
	$(MAKE) -j16 -C build/$@

install:
	$(MAKE) -C build/release install

rpm:
	$(MAKE) -j16 -C build/release package
	cp build/release/*.rpm .

doc:
	$(MAKE) -C build/release/ doc
	$(MAKE) -C build/release/doxygen/latex/
	cp build/release/doxygen/latex/refman.pdf manual.pdf
	rm -rf build/release/doxygen/latex/
	tar -cf manual.tar build/release/doxygen/
	gzip manual.tar

clean:
	$(RM) -r build
	rm -rf *.pdf *.rpm manual.tar*

count:
	wc -l `find . -iname '*.c' && find . -iname '*.hh' && find . -iname '*.cpp'`

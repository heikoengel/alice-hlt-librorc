TARGETS=release debug sim_release sim_debug

all: $(TARGETS)

$(TARGETS):
	$(MAKE) -j16 -C build/$@

clean:
	$(RM) -r build

count:
	wc -l `find . -iname '*.c' && find . -iname '*.hh' && find . -iname '*.cpp'`

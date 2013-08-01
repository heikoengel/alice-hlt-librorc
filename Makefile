TARGETS=release debug sim_release sim_debug

all: $(TARGETS)

$(TARGETS):
	$(MAKE) -j16 -C build/$@

clean:
	$(RM) -r build

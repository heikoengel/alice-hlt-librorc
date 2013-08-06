TARGETS=release debug sim_release sim_debug

all: $(TARGETS)
clean: $(TARGETS)_clean

$(TARGETS):
	$(MAKE) -j16 -C build/$@

$(TARGETS)_clean:
	$(MAKE) -j16 -C build/$@ clean

mrproper:
	$(RM) -r build

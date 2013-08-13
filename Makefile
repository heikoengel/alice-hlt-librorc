TARGETS=release sim_release

all: $(TARGETS)

$(TARGETS):
	$(MAKE) -j16 -C build/$@

clean:
	$(RM) -r build

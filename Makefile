TARGETS=release debug sim_release sim_debug

all: $(TARGETS)

$(TARGETS):
	$(MAKE) -C build/$@

clean:
	$(RM) -r build

#test

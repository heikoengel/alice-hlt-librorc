all:
	make -C librorc/
	make -C apps/

clean:
	make -C librorc/ clean
	make -C apps/ clean


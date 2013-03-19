#!/bin/bash

for size in 4 8 16 32 64 128 256 512 1024 2048 4096
do
	./build/release/pcie128pg_nch.o $size
done

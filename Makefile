CC = g++

jsonsmile: source/encoder.cpp source/runner.cpp source/decoder.cpp
	$(CC) source/encoder.cpp source/runner.cpp source/decoder.cpp -o source/runner
all: file_generator file_contaminator scanner
file_generator: file_generator.cpp
	g++ file_generator.cpp -o file_generator
file_contaminator: file_contaminator.cpp
	g++ file_contaminator.cpp -o file_contaminator
scanner: scanner.cpp
	g++ scanner.cpp -o scanner
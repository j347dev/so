simulador: src/*.hpp src/main.cpp
	g++ -std=c++17 -O2 -Iinclude src/main.cpp -o simulador
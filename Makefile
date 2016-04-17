Mshell : Mshell.cpp Mwc Mcp Mcmp Mcat
	g++ Mshell.cpp -o bin/Mshell -lreadline -lncurses -std=c++11

Mwc : Mwc.cpp
	g++ Mwc.cpp -o bin/Mwc

Mcp : Mcp.cpp
	g++ Mcp.cpp -o bin/Mcp

Mcmp : Mcmp.cpp
	g++ Mcmp.cpp -o bin/Mcmp

Mcat : Mcat.cpp
	g++ Mcat.cpp -o bin/Mcat


.PHONY : debug
debug:
	g++ -g Mshell.cpp -o bin/Mshell -lreadline -lncurses -std=c++11
	g++ -g Mwc.cpp -o bin/Mwc
	g++ -g Mcmp.cpp -o bin/Mcmp
	g++ -g Mcp.cpp -o bin/Mcp
	g++ -g Mman.cpp -o bin/Mman -lncurses
	g++ -g  Mcat.cpp -o bin/Mcat

.PHONY : clean
clean:
	rm -rf bin/*


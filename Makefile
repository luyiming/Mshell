Mshell : Mshell.cpp Mwc Mcp
	g++ Mshell.cpp -o bin/Mshell -lreadline -std=c++11

Mwc : Mwc.cpp
	g++ Mwc.cpp -o bin/Mwc

Mcp : Mcp.cpp
	g++ Mcp.cpp -o bin/Mcp

.PHONY : debug
debug:
	g++ -g Mshell.cpp -o bin/Mshell
	g++ -g Mwc.cpp -o bin/Mwc


.PHONY : clean
clean:
	rm -rf bin/*


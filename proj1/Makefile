all: wireview

wireview: wireview.cpp host.cpp host.hpp hostlist.cpp hostlist.hpp info.hpp info.cpp misc.hpp misc.cpp
	g++ -Wall --pedantic -std=c++11 -lpcap wireview.cpp host.cpp hostlist.cpp info.cpp misc.cpp -o wireview

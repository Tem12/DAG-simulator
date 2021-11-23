#BOOST_LIBS=-lboost_program_options-mt -lboost_system-mt -lboost_thread-mt -lboost_chrono-mt
#BOOST_LIBS=-lboost_program_options -lboost_system -lboost_thread -lboost_chrono
BOOST_LIBS=-lboost_program_options -lboost_system -lboost_thread -lboost_chrono
CFLAGS=-O2
#CFLAGS=-g -ggdb -DTRACE
# CFLAGS=-g -ggdb

scheduler: main.cpp scheduler.cpp
	clang++ -std=c++11 -I/usr/local/include -L/usr/local/lib $(CFLAGS) -o mining_simulator main.cpp scheduler.cpp est_time.cpp $(BOOST_LIBS)

#Default Flags
FLAGS = -Wall -Wextra -Wconversion -Wvla -pedantic -std=c++11

# make release
release: FLAGS += -O3
release: all

# make debug
debug: FLAGS += -g
debug: all

# make profile - time 
profile: FLAGS += -O3 -pg
profile: all

# all
all: build_json build_p2
	g++ $(FLAGS) jsoncpp.o p2.o -o p2

build_json: jsoncpp.cpp
	g++ $(FLAGS) -c jsoncpp.cpp
build_p2: p2.cpp
	g++ $(FLAGS) -c p2.cpp

# make clean
clean:
	rm -f *.o ./p2 gmon.out

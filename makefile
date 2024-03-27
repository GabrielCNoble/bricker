#CC				:= gcc
CC				:= x86_64-w64-mingw32-gcc
CPP				:= g++
C_SRC 			:= $(wildcard ./*.c)
C_OBJ 			:= $(C_SRC:.c=.o)
C_FLAGS			:= -g -std=c2x -O3
LIB_BIN 		:= $(wildcard libs/*/lib/windows)
LIB_INC 		:= $(wildcard libs/*/include)
LIB_SRC_PATH 	:= $(wildcard libs/*/src)
LIB_CPP_SRC		:= $(foreach path, $(LIB_SRC_PATH), $(wildcard $(path)/*.cpp) $(wildcard $(path)/**/*.cpp))
LIB_C_SRC		:= $(foreach path, $(LIB_SRC_PATH), $(wildcard $(path)/*.c) $(wildcard $(path)/**/*.c))
LIB_OBJ			:= $(LIB_CPP_SRC:.cpp=.o) $(LIB_C_SRC:.c=.o)
#EXEC_NAME		:= bricker.out
EXEC_NAME		:= bricker.exe


%.o: %.cpp
	$(CPP) -c $< $(foreach path, $(LIB_INC), -I $(path)) -o $@

%.o: %.c
	$(CC) -c $< $(foreach path, $(LIB_INC), -I $(path)) $(C_FLAGS) -o $@


all: $(C_OBJ) $(foreach lib_obj, $(LIB_OBJ), $(lib_obj))
	$(CC) $^ -Wl,--copy-dt-needed-entries $(foreach path, $(LIB_BIN), -L $(path) -Wl,-rpath=$(path)) -lSDL2main -lSDL2 -lSDL2_ttf -lopenal -lstdc++ -o $(EXEC_NAME)

clean: 
	rm -f $(C_OBJ) $(LIB_OBJ)

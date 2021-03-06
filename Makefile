CC = gcc
CPP = g++
LD = g++
CFLAGS = $(FLAGS)
CPPFLAGS = $(FLAGS)
BIN_NAME = orcs
RM = rm -f

#FLAGS =   -O3 -ggdb -Wall -Wextra -Werror
FLAGS = -ggdb -Wall
LDFLAGS = -ggdb

########################################################################

LIBRARY = -lz

SRC_PACKAGE = 		opcode_package.cpp 

SRC_TRACE_READER = 	trace_reader.cpp

SRC_PROCESSOR =	 	processor.cpp 

SRC_CORE =  simulator.cpp orcs_engine.cpp\
			$(SRC_TRACE_READER)	\
			$(SRC_PACKAGE) \
			stride.cpp \
			cache.cpp \
			branch_predictor.cpp \
			prefetcher.cpp \
			btb.cpp \
			$(SRC_PROCESSOR)

########################################################
OBJS_CORE = ${SRC_CORE:.cpp=.o}
OBJS = $(OBJS_CORE)
########################################################
# implicit rules
%.o : %.cpp %.hpp
	$(CPP) -c $(CPPFLAGS) $< -o $@

########################################################

all: orcs

orcs: $(OBJS_CORE)
	$(LD) $(LDFLAGS) -o $(BIN_NAME) $(OBJS) $(LIBRARY)

clean:
	-$(RM) $(OBJS)
	-$(RM) $(BIN_NAME)
	@echo OrCS cleaned!
	@echo

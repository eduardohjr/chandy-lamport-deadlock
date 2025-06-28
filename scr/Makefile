CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

# Source files
PHILOSOPHER_SRCS = philosopher.cpp message.cpp snapshot.cpp
COORDINATOR_SRCS = coordinator.cpp message.cpp snapshot.cpp

# Object files
PHILOSOPHER_OBJS = $(PHILOSOPHER_SRCS:.cpp=.o)
COORDINATOR_OBJS = $(COORDINATOR_SRCS:.cpp=.o)

# Executables
PHILOSOPHER_BIN = philosopher
COORDINATOR_BIN = coordinator

.PHONY: all clean

all: $(PHILOSOPHER_BIN) $(COORDINATOR_BIN)

$(PHILOSOPHER_BIN): $(PHILOSOPHER_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(COORDINATOR_BIN): $(COORDINATOR_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(PHILOSOPHER_OBJS) $(COORDINATOR_OBJS) $(PHILOSOPHER_BIN) $(COORDINATOR_BIN)
SRCDIR   = src
BINDIR   = bin

# Base directories for code
CXXINCDIRS = -I $(SRCDIR)
# Base directories for libraries - in addition to the default system location
CXXLIBDIRS = 
# Optional warning flags
CXXWARN    = -W -Wall -Wextra -Wshadow -Wfloat-equal -Wswitch-default -Wno-unused-parameter

CXX      = g++
CXXFLAGS = -g $(CXXWARN) $(CXXINCDIRS) $(CXXLIBDIRS) -std=c++11 -pthread -O3
LDFLAGS  = 

CSRCS   = $(shell find $(SRCDIR)/client -type f -name '*.cpp')
COBJS   = $(foreach file, $(patsubst %.cpp,%.o,$(CSRCS)), $(subst $(SRCDIR)/,$(BINDIR)/,$(file)))
SRCS    = $(shell find $(SRCDIR)/common -type f -name '*.cpp')
OBJS    = $(foreach file, $(patsubst %.cpp,%.o,$(SRCS)), $(subst $(SRCDIR)/,$(BINDIR)/,$(file)))
SSRCS   = $(shell find $(SRCDIR)/server -type f -name '*.cpp')
SOBJS   = $(foreach file, $(patsubst %.cpp,%.o,$(SSRCS)), $(subst $(SRCDIR)/,$(BINDIR)/,$(file)))

.PHONY: clean all dirs

all: client server

# Targets

client: $(SRCDIR)/client/pubsub_client.cpp $(COBJS) $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(COBJS) $(OBJS) $(LDFLAGS)

server: $(SRCDIR)/server/pubsub_server.cpp $(SOBJS) $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(SOBJS) $(OBJS) $(LDFLAGS)

# Compiler

$(SOBJS) $(COBJS) $(OBJS): $(BINDIR)/%.o: $(SRCDIR)/%.cpp $(wildcard $(SRCDIR)/%.h) | dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo target is $@, depens are $^

# Directory-maker

dirs:
	@mkdir -p $(BINDIR)
	@find $(SRCDIR) -type d -print | sed 's/$(SRCDIR)/$(BINDIR)/g' | xargs -n 1 mkdir -p 

clean:
	rm -rf $(BINDIR)


EXECUTABLE  = market_simulator


PROJECTFILE = $(or $(wildcard project*.cpp $(EXECUTABLE).cpp), main.cpp)

# disable built-in rules
.SUFFIXES:

# designate which compiler to use
CXX         = g++

# rule for creating objects
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $*.cpp

TESTSOURCES = $(wildcard test*.cpp)

SOURCES     = $(wildcard *.cpp)
SOURCES     := $(filter-out $(TESTSOURCES), $(SOURCES))
OBJECTS     = $(SOURCES:%.cpp=%.o)


CXXFLAGS = -std=c++17 -Wconversion -Wall -Werror -Wextra -pedantic


debug: CXXFLAGS += -g3 -DDEBUG -fsanitize=address -fsanitize=undefined -D_GLIBCXX_DEBUG
debug:
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(EXECUTABLE)_debug
.PHONY: debug


release: CXXFLAGS += -O3 -DNDEBUG
release: $(EXECUTABLE)
.PHONY: release


valgrind: CXXFLAGS += -g3
valgrind:
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(EXECUTABLE)_valgrind
.PHONY: valgrind

profile: CXXFLAGS += -g3
profile:
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(EXECUTABLE)_profile
.PHONY: profile

gprof: CXXFLAGS += -pg
gprof:
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(EXECUTABLE)_profile
.PHONY: gprof


static:
	cppcheck --enable=all --suppress=missingIncludeSystem \
      $(SOURCES) *.h *.hpp
.PHONY: static




NO_IDENTIFIER = xcode_redirect.hpp,getopt.h,getopt.c,xgetopt.h


identifier: $(foreach tsrc,$(TESTSOURCES),$(eval NO_IDENTIFIER := $(NO_IDENTIFIER),$(tsrc)))
identifier:
	@if [ $$(grep --include=*.{h,hpp,c,cpp} --exclude={$(NO_IDENTIFIER)} --directories=skip -L $(IDENTIFIER) * | wc -l) -ne 0 ]; then \
		printf "Missing project identifier in file(s): "; \
		echo `grep --include=*.{h,hpp,c,cpp} --exclude={$(NO_IDENTIFIER)} --directories=skip -L $(IDENTIFIER) *`; \
		exit 1; \
	else \
		rm -f $(PARTIAL_SUBMITFILE) $(FULL_SUBMITFILE) $(UNGRADED_SUBMITFILE); \
	fi
.PHONY: identifier


all: debug release
all: profile valgrind
.PHONY: all

$(EXECUTABLE): $(OBJECTS)
ifneq ($(EXECUTABLE), executable)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(EXECUTABLE)
else
	@echo Edit EXECUTABLE variable in Makefile.
	@echo Using default a.out.
	$(CXX) $(CXXFLAGS) $(OBJECTS)
endif








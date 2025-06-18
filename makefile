.DEFAULT_GOAL := all
.DELETE_ON_ERROR:
.SUFFIXES:

cpps := $(wildcard *.cpp)
-include $(cpps:.cpp=.d)
CXXFLAGS += @compile_flags.txt
CPPFLAGS += -MMD -MP

%: %.o
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<

.PHONY: clean all
all: main
clean:
	rm -fr $(.DEFAULT_GOAL) *.{o,d,dSYM} compile_commands.json

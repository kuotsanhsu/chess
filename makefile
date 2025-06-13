.DEFAULT_GOAL := main
.SUFFIXES:
.SUFFIXES: .cpp .o

srcs := $(wildcard *.cpp)
objs := $(srcs:.cpp=.o)
deps := $(objs:.o=.d)
-include $(deps)
CXXFLAGS += -std=c++23 -fno-exceptions -Wno-c23-extensions
CPPFLAGS += -MMD -MP -Werror -Wpointer-arith -Wimplicit-fallthrough
LDFLAGS += -flto=thin

main: main.o
main:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

.PHONY: clean cleanall
clean:
	$(RM) $(objs) $(objs:.o=)
cleanall: clean
	$(RM) $(deps)
	rm -fr $(wildcard *.dSYM)

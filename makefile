.DEFAULT_GOAL := main
.SUFFIXES: .cpp .o

srcs := $(wildcard *.cpp)
objs := $(srcs:.cpp=.o)
deps := $(objs:.o=.d)
-include $(deps)
CXXFLAGS += @compile_flags.txt
CPPFLAGS += -MMD -MP
# CPPFLAGS += -MMD -MP -MJ compile_commands.json
# LDFLAGS += -flto=thin

%.pcm: CXXFLAGS += --precompile
%.pcm: %.cppm
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) $(OUTPUT_OPTION) $<
%.o: %.pcm
	$(CXX) $(CXXFLAGS) $(TARGET_ARCH) -c $(OUTPUT_OPTION) $<
%: %.o
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@
%: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

main: main.cpp chess.pcm ansi.pcm
hello: hello.cpp greet.pcm

# %.o: %.cpp
# 	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c $(OUTPUT_OPTION) $<
# %: %.cpp
# 	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@
# %: %.o
# 	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

.PHONY: clean cleanall
clean:
	rm -fr *.pcm *.o *.d *.dSYM

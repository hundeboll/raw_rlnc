CXXFLAGS := ${CXXFLAGS} -std=c++11 -DBOOST_NO_INTRINSIC_WCHAR_T -D_GLIBCXX_USE_NANOSLEEP -g -O2 -ftree-vectorize -s
INCLUDES = -Isrc/ \
	   -I../kodo/src/ \
	   -I../kodo/bundle_dependencies/boost-91e411/master/ \
	   -I../kodo/bundle_dependencies/fifi-6ca972/master/src/ \
	   -I../kodo/bundle_dependencies/sak-2baed8/master/src/ \
	   -I../kodo/bundle_dependencies/gauge-d53326/master/src/ \

DEPENDS = src/*.hpp
TARGETS = encoder decoder helper recoder source destination relay

all: $(TARGETS)

.PHONY: clean

$(TARGETS): %: src/%.cpp $(DEPENDS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(INCLUDES) $< -o $@

clean:
	rm -f ${TARGETS}

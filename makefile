all : $(patsubst src/%.cpp,bin/%,$(wildcard src/*.cpp))

FORCE :

.makefile.helper : scripts/makefile_helper.sh working src media
	scripts/makefile_helper.sh > $@

include .makefile.helper	

CPPFLAGS += -W -Wall
CPPFLAGS += -Iinc -std=c++14 -g

CPPFLAGS += -O3 -DNDEBUG=1

LDLIBS += -ljpeg

HEADER_DEPS = $(wildcard inc/*.hpp)

bin/% : src/%.cpp $(HEADER_DEPS)
	-mkdir -p bin
	$(CXX) $(CPPFLAGS) $(filter-out %.hpp,$^) -o $@ $(LDFLAGS) $(LDLIBS)

working :
	mkdir working

working/%.mjpeg : media/%.mp4
	mkdir -p working
	scripts/video_to_mjpeg.sh $< > $@

working/%.mjpeg.play : working/%.mjpeg FORCE
	scripts/mjpeg_to_play.sh < $<



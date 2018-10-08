CXX = g++
CFLAGS = -std=c++17
VERSION=1.0.1
LIB_NAME=text-gl


all: lib/lib$(LIB_NAME).so.$(VERSION)

clean:
	rm -f bin/test_visual bin/test_encoding lib/lib$(LIB_NAME).so.$(VERSION) obj/*.o core


test: bin/test_visual bin/test_encoding
	bin/test_encoding
	bin/test_visual data/sample1.svg
	bin/test_visual data/sample2.svg


bin/test_visual: tests/visual.cpp lib/lib$(LIB_NAME).so.$(VERSION)
	mkdir -p bin
	$(CXX) $(CFLAGS) -I include $^ -lboost_filesystem -lboost_system -lGL -lGLEW -lSDL2 -o $@


bin/test_encoding: tests/encoding.cpp lib/lib$(LIB_NAME).so.$(VERSION)
	mkdir -p bin
	$(CXX) $(CFLAGS) -I include  -fexec-charset=UTF-8 $^ -lboost_unit_test_framework -o $@


lib/lib$(LIB_NAME).so.$(VERSION): obj/parse.o obj/image.o obj/utf8.o obj/error.o obj/tex.o obj/text.o
	mkdir -p lib
	$(CXX) $(CFLAGS) $^ -lGL -lxml2 -lcairo -o $@ -fPIC -shared


obj/%.o: src/%.cpp  include/text-gl/font.h include/text-gl/text.h include/text-gl/utf8.h
	mkdir -p obj
	$(CXX) $(CFLAGS) -I include/text-gl -c $< -o $@ -fPIC

install: lib/lib$(LIB_NAME).so.$(VERSION)
	/usr/bin/install -d -m755 /usr/local/lib
	/usr/bin/install -d -m755 /usr/local/include/text-gl
	/usr/bin/install -m644 lib/lib$(LIB_NAME).so.$(VERSION) /usr/local/lib/lib$(LIB_NAME).so.$(VERSION)
	ln -sf /usr/local/lib/lib$(LIB_NAME).so.$(VERSION) /usr/local/lib/lib$(LIB_NAME).so
	/usr/bin/install -D include/text-gl/*.h /usr/local/include/text-gl/

uninstall:
	rm -f /usr/local/lib/lib$(LIB_NAME).so* /usr/local/include/text-gl/*.h

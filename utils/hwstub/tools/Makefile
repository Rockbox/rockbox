CC=gcc
CXX=g++
LD=g++
HWSTUB_INCLUDE_DIR=../include
HWSTUB_LIB_DIR=../lib
REGTOOLS_INCLUDE_DIR=../../regtools/include
REGTOOLS_LIB_DIR=../../regtools/lib
INCLUDES=-I$(HWSTUB_INCLUDE_DIR) -I$(REGTOOLS_INCLUDE_DIR) `pkg-config --cflags lua5.2` `pkg-config --cflags libusb-1.0`
CFLAGS=-Wall -O2  -std=c99 -g $(INCLUDES)  -D_XOPEN_SOURCE=600
CXXFLAGS=-Wall -O2 -std=c++11 -g $(INCLUDES)
LDFLAGS=`pkg-config --libs libusb-1.0` `pkg-config --libs lua5.2` -lreadline -L$(HWSTUB_LIB_DIR) -L$(REGTOOLS_LIB_DIR) -lsocdesc -lhwstub `xml2-config --libs` -pthread
EXEC=hwstub_shell hwstub_load hwstub_server hwstub_test
SRC=$(wildcard *.c)
SRCXX=$(wildcard *.cpp)
OBJ=$(SRC:.c=.o) $(SRCXX:.cpp=.o)
LIBS=$(HWSTUB_LIB_DIR)/libhwstub.a $(REGTOOLS_LIB_DIR)/libsocdesc.a

all: $(EXEC)

$(HWSTUB_LIB_DIR)/libhwstub.a:
	make -C $(HWSTUB_LIB_DIR)

$(REGTOOLS_LIB_DIR)/libsocdesc.a:
	make -C $(REGTOOLS_LIB_DIR)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

hwstub_shell: hwstub_shell.o prompt.o $(LIBS)
	$(LD) -o $@ $^ $(LDFLAGS)

hwstub_load: hwstub_load.o $(LIBS)
	$(LD) -o $@ $^ $(LDFLAGS)

hwstub_server: hwstub_server.o $(LIBS)
	$(LD) -o $@ $^ $(LDFLAGS)

hwstub_test: hwstub_test.o $(LIBS)
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(OBJ) $(LIB) $(EXEC)



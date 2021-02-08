INCLUDE_PATH = -I /home/xjw/tools/third_part/include -I /usr/include/jsoncpp
LIB_PATH = -L /home/xjw/tools/third_part/lib -lctemplate -lpthread -ljsoncpp

BIN = ../bin/oj_server

$(BIN):oj_server.cpp
	g++ $^ -o $@ -g $(INCLUDE_PATH) $(LIB_PATH)
	
.PHONY:clean
clean:
	rm $(BIN)

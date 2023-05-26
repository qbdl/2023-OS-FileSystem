CXX=g++
CXX_FLAGS=-c -std=c++11 -g

TARGET=init_ client_#生成文件

#源文件和对象文件
INIT_SRCS=src/init.cpp
CLIENT_SRCS=src/BlockCache.cpp src/client.cpp src/fs.cpp src/inode.cpp src/server.cpp

INIT_OBJS=$(INIT_SRCS:.cpp=.o)
CLIENT_OBJS=$(CLIENT_SRCS:.cpp=.o)

#依赖文件
DEPS=$(wildcard include/*.h)

# 声明了伪目标all和clean，避免因文件名与目标同名而导致的错误
.PHONY: all init_ client_ clean

#默认目标
all: $(TARGET)

#两个目标，链接目标文件生成目标程序
init_:  $(INIT_OBJS)
		$(CXX) $(INIT_OBJS) -o init
		rm -f src/*.o
client_:$(CLIENT_OBJS)
		$(CXX) $(CLIENT_OBJS) -o client
		rm -f src/*.o

#编译源文件生成目标文件
%.o: %.cpp $(DEPS)
	$(CXX) $(CXX_FLAGS) $< -o $@

#清除目标
clean:
	rm -f $(OBJS) $(TARGET)
	rm -f *.img
	rm -f init
	rm -f client

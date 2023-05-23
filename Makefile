CC=g++
CFLAGS=-c -std=c++11

TARGET=init #生成文件

#源文件和对象文件
SRCS=$(wildcard src/*.cpp)
# SRCS=src/init.cpp src/initDir.cpp
OBJS=$(SRCS:.cpp=.o)

#依赖文件
DEPS=$(wildcard include/*.h)

# 声明了伪目标all和clean，避免因文件名与目标同名而导致的错误
.PHONY: all clean

#默认目标
all: $(TARGET)
	rm -f src/*.o
#链接目标文件生成目标程序
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

#编译源文件生成目标文件
%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) $< -o $@

#清除目标
clean:
	rm -f $(OBJS) $(TARGET)
	rm -f *.img
	rm -f init

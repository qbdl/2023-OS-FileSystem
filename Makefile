CC=g++
CFLAGS=-c -std=c++11

#生成文件
TARGET=init

#
SRCS=init.cpp initDir.cpp

OBJS=$(SRCS:.cpp=.o)

#依赖文件
DEPS=diskInode.h superBlock.h directory.h initDir.h parameter.h

all: $(TARGET)
	rm -f *.o

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

# 生成目标文件的依赖关系
%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	rm -f *.img

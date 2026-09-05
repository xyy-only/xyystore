CC=g++
CFLAGS=-g -Wall
target=client
src=$(wildcard *.cpp)
obj=$(patsubst %.cpp, %.o, $(src))
link=-lpthread -lcrypt -lcrypto -lmysqlclient
$(target):$(obj)
	$(CC) $(CFLAGS)  $^ -o $@ $(link)
%.o:%.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(link)
clean:
	rm -rf $(target) $(obj)

bin = httpserver
cc = g++
LD_FLAGS = -std=c++11 -lpthread -D DEBUG_SHOW
src = main.cc

$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAGS)

.PHONY:clean
clean:
	rm -f $(bin)

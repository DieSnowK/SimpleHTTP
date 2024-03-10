bin = httpserver
cgi = test_cgi
cc = g++
GLD_FLAGS = -std=c++11 -D DEBUG_SHOW
LD_FLAGS = $(GLD_FLAGS) -lpthread
src = main.cc
curr = $(shell pwd)

.PHONY:ALL
ALL:$(bin) $(cgi)

$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAGS)

$(cgi):$(curr)/CGI/*.cc
	$(cc) -o $@ $^ $(GLD_FLAGS)

.PHONY:output
output:
	mkdir output
	cp $(bin) output
	cp -rf wwwroot output
	cp $(cgi) output/wwwroot

.PHONY:clean
clean:
	rm -f $(bin) $(cgi)
	rm -rf output
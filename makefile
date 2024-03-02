bin = httpserver
cgi = test_cgi
cc = g++
LD_FLAGS = -std=c++11 -lpthread -D DEBUG_SHOW
src = main.cc
curr = $(shell pwd)

.PHONY:ALL
ALL:$(bin) $(cgi)

$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAGS)

$(cgi):$(curr)/CGI/*.cc
	$(cc) -o $@ $^

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
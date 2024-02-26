bin=httpserver
cc=g++
LD_FLAGS=-std=c++11 -lpthread #-DDEBUG=1
curr=$(shell pwd)
src=main.cc

.PHONY:ALL
ALL:$(bin) CGI

$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAGS)

CGI:
	cd $(curr)/cgi;\
		make;\
		cd -;

.PHONY:clean
clean:
	rm -f $(bin)
	rm -rf output
	cd $(curr)/cgi;\
		make clean;\
		cd -

.PHONY:output
output:
	mkdir -p output
	cp $(bin) output
	cp -rf wwwroot output
	cp ./cgi/test_cgi output/wwwroot
	cp ./cgi/shell_cgi.sh output/wwwroot
	cp ./cgi/python_cgi.py output/wwwroot
	cp ./cgi/mysql_cgi output/wwwroot


All:
	cd Caravel&&make
	cd fastore&&make
	cd thrift&&make
	cd Proxy&&make
	cd Server&&make

remake:
	cd Caravel&&make clean && make
	cd fastore&&make clean && make
	cd thrift&&make clean && make
	cd Proxy&&make clean && make
	cd Server&&make clean && make
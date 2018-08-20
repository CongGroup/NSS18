CC=g++
BIN=oreTest test
FLAGS=-Wall -g -fpermissive -maes -std=c++11
RM=-rm -f
SRC=$(wildcard *.cpp)
OBJ=$(SRC:.cpp=.o)
#OBJ=$(patsubst %.cpp, %.o, $(SRC))
COMMON_OBJ=$(filter-out oreTest.o test.o, $(OBJ))

#thrift
#THRIFT_LINK=-lthrift
#THRIFT_LIB=-L/usr/local/lib
#THRIFT_INC=-I/usr/local/inc

#openssl
OPENSSL_LINK=-lcrypto

#caravel
CARAVEL_LINK=-lcaravel
CARAVEL_LIB=-L../Caravel
CARAVEL_INC=-I../Caravel

#redis3m
#REDIS3M_LINK=-lredis3m 
#REDIS3M_LIB=-L/usr/local
#REDIS3M_INC=-I/usr/local/include

#fastore
FASTORE_LINK=-lfastore
FASTORE_LIB=-L../fastore
FASTORE_INC=-I../fastore

#combine the inc & lib
INC=$(OPENSSL_INC) $(THRIFT_INC) $(CARAVEL_INC) $(PAILLIER_INC) $(REDIS3M_INC) $(FASTORE_INC)
LIB=$(FASTORE_LINK) $(FASTORE_LIB) $(CARAVEL_LINK) $(CARAVEL_LIB) $(THRIFT_LINK) $(THRIFT_LIB) $(OPENSSL_LINK) $(REDIS3M_LINK) 


all: $(BIN)
	$(warning $(OBJ))

%.d: %.cpp
	$(CC) -MM $(FLAGS) $(INC) $< -o $@

%.o: %.cpp %.d
	$(CC) -c $(FLAGS) $(INC) $< -o $@

oreTest: $(OBJ) $^
	$(CC) $(FLAGS) -o $@ $@.o $(COMMON_OBJ) $(LIB) 

test: $(OBJ) $^
	$(CC) $(FLAGS) -o $@ $@.o $(COMMON_OBJ) $(LIB) 

clean:
	$(RM) *.o
	$(RM) *.d
	$(RM) $(BIN)

-include $(SRC:.cpp=.d)

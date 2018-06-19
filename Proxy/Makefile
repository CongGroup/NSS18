CC=g++
BIN=main
FLAGS=-Wall -g -fpermissive -std=c++11 -maes -O3
RM=-rm -f
SRC=$(wildcard *.cpp)
#OBJ=$(SRC:*.cpp=*.o)
OBJ=$(patsubst %.cpp, %.o, $(SRC))
COMMON_OBJ=$(filter-out main.o, $(OBJ))

#thrift
THRIFT_LINK=-lthrift
THRIFT_LIB=-L/usr/local/lib
THRIFT_INC=-I/usr/local/inc

#openssl
OPENSSL_LINK=-lcrypto

#caravel
CARAVEL_LINK=-lcaravel
CARAVEL_LIB=-L../Caravel
CARAVEL_INC=-I../Caravel

#redis3m
REDIS3M_LINK=-lredis3m 
REDIS3M_LIB=-L/usr/local
REDIS3M_INC=-I/usr/local/include

#fastore
FASTORE_LINK=-lfastore
FASTORE_LIB=-L../fastore
FASTORE_INC=-I../fastore

#dbService
DB_INC=-I../thrift
DB_LIB=-L../thrift -ldbservice

#combine the inc & lib
INC=$(OPENSSL_INC) $(THRIFT_INC) $(CARAVEL_INC) $(PAILLIER_INC) $(REDIS3M_INC) $(FASTORE_INC) $(DB_INC)
LIB=$(FASTORE_LINK) $(FASTORE_LIB) $(CARAVEL_LINK) $(CARAVEL_LIB) $(THRIFT_LINK) $(THRIFT_LIB) $(OPENSSL_LINK) $(REDIS3M_LINK) $(DB_LIB) 


all: $(BIN)

%.d: %.cpp
	$(CC) -MM $(FLAGS) $(INC) $< -o $@

%.o: %.cpp %.d
	$(CC) -c $(FLAGS) $(INC) $< -o $@

main:$(OBJ)
	$(CC) $(FLAGS) -o proxy $@.o $(COMMON_OBJ) $(LIB)



clean:
	$(RM) proxy
	$(RM) *.o
	$(RM) *.d
	$(RM) $(BIN)

-include $(SRC:.cpp=.d)

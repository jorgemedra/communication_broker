
#********************************
# Compile with debug option:
#	make DBGFLG=-g
#********************************

TARGET 	=		comserv 
SRC_DIR	= 		./src
OBJ_DIR = 		./obj
BIN_DIR = 		./bin

OBJ_FLS = 		$(OBJ_DIR)/main.o \
				$(OBJ_DIR)/ntwrk_basic.o \
				$(OBJ_DIR)/wsserver.o \
				$(OBJ_DIR)/wscnx.o \
				$(OBJ_DIR)/tcpserver.o \
				$(OBJ_DIR)/tcpcnx.o \
				$(OBJ_DIR)/stomp_server.o \
				$(OBJ_DIR)/stomp_server_proccess.o \
				$(OBJ_DIR)/stomp_server_send.o \
				$(OBJ_DIR)/stomp_protocol.o \
				$(OBJ_DIR)/stomp_session.o \
				$(OBJ_DIR)/jomt_util.o \
				$(OBJ_DIR)/stomp_subscription.o
				

CC 			= g++
GDB			= gdb
#LFLAGS		= -pthread -lboost_log -lboost_thread -lboost_log_setup -lstdc++
LFLAGS		= -L/usr/local/opt/openssl/lib -lstdc++ -pthread -lssl -lcrypto
CPPFLAGS	= -c -std=c++17 -Wall -MD -I /usr/local/opt/openssl/include
#To define the max connection allowed in this Sample
MAX_CNXS	= -DCNX_TCP_MAX=1005 -DCNX_ADM=5 -DFD_SET_SIZE=1010

ifdef DBGFLG
echo Debug Flag = $(DBGFLG)
override CPPFLAGS += $(DBGFLG)
endif

all: build
	@echo "Cmpile Flags: " $(CPPFLAGS)

help:
	@echo "............................................."
	@echo "Build test: make"
	@echo "Build and activate debug: make DBGFLG=-g"
	@echo "Clean compiled files: make clean"

build: $(OBJ_DIR) $(OBJ_FLS) $(BIN_DIR)
	@echo "Building..."
	$(CC) $(LFLAGS) -o $(BIN_DIR)/$(TARGET) $(OBJ_FLS)
	@echo "Build finished."

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(MAX_CNXS) $(CPPFLAGS) $< -o $@

-include $(OBJ_DIR)/*.d

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR) 

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)


clean:
	@echo "Cleaning..."
	@rm -rf $(OBJ_DIR)
	@rm -rf $(BIN_DIR)
	@rm -f $(SRC_DIR)/*.o
	@rm -f $(SRC_DIR)/*.d
	@rm -f $(LOG_DIR)/*.*
	@echo "Cleaned"
# SOURCES
WORKDIR=$(shell pwd)
WORKDIR_HELPERS=$(WORKDIR)/helpers

# HEADERS
WORKDIR_H=$(WORKDIR)/include
WORKDIR_HELPERS_H=$(WORKDIR)/include/helpers

# BINARIES
WORKDIR_BINARIES=$(WORKDIR)/bin

# COMPILER
CC=gcc

# FLAGS OF COMPILER
BACKUP_FLAGS=-c -g -O2

all: build

build_logger_api:
	$(CC) $(BACKUP_FLAGS) $(WORKDIR_HELPERS)/helpers_logger.c -o \
	$(WORKDIR_BINARIES)/helpers_logger.o

build_hash_table_api: build_logger_api 
	$(CC) $(BACKUP_FLAGS) $(WORKDIR_HELPERS)/helpers_hash_table.c -o \
	$(WORKDIR_BINARIES)/helpers_hash_table.o

build_event_queue: build_hash_table_api
	$(CC) $(BACKUP_FLAGS) $(WORKDIR_HELPERS)/event_queue.c -o \
	$(WORKDIR_BINARIES)/event_queue.o 

build_helpers: build_event_queue
	$(CC) $(BACKUP_FLAGS) -lpthread $(WORKDIR_HELPERS)/helpers_common.c -o \
    $(WORKDIR_BINARIES)/helpers_common.o 

build_main_loop: build_helpers
	$(CC) $(BACKUP_FLAGS) -lpthread $(WORKDIR)/main.c -o \
	$(WORKDIR_BINARIES)/main.o

build: build_main_loop
	$(CC) $(WORKDIR_BINARIES)/main.o $(WORKDIR_BINARIES)/event_queue.o $(WORKDIR_BINARIES)/helpers_hash_table.o $(WORKDIR_BINARIES)/helpers_logger.o $(WORKDIR_BINARIES)/helpers_common.o -o $(WORKDIR_BINARIES)/backupd
	bash integration.sh
	rm -f $(WORKDIR_BINARIES)/*.o

clean:
	 rm -f $(WORKDIR_BINARIES)/* ~/backup_service.log *.log 

CC = gcc
CFLAGS = -O2
ALL_CFLAGS = -Wall -fdiagnostics-color=always $(CFLAGS) -I.
SANITIZE_FLAGS = -fsanitize=address,undefined -g -fno-omit-frame-pointer

OUT_DIR = bin

all: pre-build lib.o md5 slave view

sanitize: CFLAGS += $(SANITIZE_FLAGS)
sanitize: all

pre-build:
	@mkdir -p $(OUT_DIR)

lib.o: lib.c lib.h
	$(CC) -c $< $(ALL_CFLAGS) -o $(OUT_DIR)/$@

md5: app.c bin/lib.o
	$(CC) $^ $(ALL_CFLAGS) -o $(OUT_DIR)/$@

slave: slave.c bin/lib.o
	$(CC) $^ $(ALL_CFLAGS) -o $(OUT_DIR)/$@

view: view.c bin/lib.o
	$(CC) $^ $(ALL_CFLAGS) -o $(OUT_DIR)/$@

clean:
	@rm -rf $(OUT_DIR)/* report.tasks $(OUT_DIR)
	@rm -rf output.txt
	@rm -rf out.txt
	@rm -f *.o

.PHONY: all clean sanitize
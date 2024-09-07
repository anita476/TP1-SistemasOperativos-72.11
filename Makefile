CC = gcc
CFLAGS = -O2
ALL_CFLAGS = -Wall -fdiagnostics-color=always $(CFLAGS)

OUT_DIR = bin

all: pre-build app slave view

app: app.c
	$(CC) $< $(ALL_CFLAGS) -o $(OUT_DIR)/$@

pre-build:
	@mkdir -p $(OUT_DIR)

slave: slave.c
	$(CC) $< $(ALL_CFLAGS) -o $(OUT_DIR)/$@

clean:
	@rm -rf $(OUT_DIR)/* report.tasks $(OUT_DIR)
	@rm -rf output.txt

.PHONY: all clean
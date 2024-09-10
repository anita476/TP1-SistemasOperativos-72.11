CC = gcc
CFLAGS = -O2
ALL_CFLAGS = -Wall -fdiagnostics-color=always -fsanitize=address -g $(CFLAGS)

OUT_DIR = bin

all: pre-build app slave view

pre-build:
	@mkdir -p $(OUT_DIR)

app: app.c
	$(CC) $< $(ALL_CFLAGS) -o $(OUT_DIR)/$@

slave: slave.c
	$(CC) $< $(ALL_CFLAGS) -o $(OUT_DIR)/$@

view: view.c
	$(CC) $< $(ALL_CFLAGS) -o $(OUT_DIR)/$@

clean:
	@rm -rf $(OUT_DIR)/* report.tasks $(OUT_DIR)
	@rm -rf output.txt
	@rm -rf out.txt

.PHONY: all clean
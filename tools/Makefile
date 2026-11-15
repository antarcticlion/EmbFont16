# tools/Makefile

CC ?= cc
CFLAGS = -std=c99 -O2 -Wall -I./src -I./stb -I../src/include
LDFLAGS = -lm

BIN_DIR = bin
SRC_DIR = src
COMMON_HDRS = ../src/include/embfont16.h ../src/include/embfont16_common.h

.DEFAULT_GOAL := all

# ラベル用フォントのソースパス (リポジトリルートからの相対パスを解決)
LABEL_FONT_SRC = ../assets/yaft/fonts/milkjf/milkjf_8x16.bdf
LABEL_FONT_HEADER = $(SRC_DIR)/label_font.h

TARGETS = $(BIN_DIR)/bdf2c \
	  $(BIN_DIR)/font_builder \
          $(BIN_DIR)/gen_map \
          $(BIN_DIR)/hex2bdf \
          $(BIN_DIR)/viewer \
          $(BIN_DIR)/bdf_recode \
          $(BIN_DIR)/bdf_cut \
          $(BIN_DIR)/bdf_diff \
          $(BIN_DIR)/bdf_remap \
          $(BIN_DIR)/bdf_jis2eucjp

# Rebuild everything when this Makefile changes.
$(TARGETS) $(LABEL_FONT_HEADER): Makefile


all: $(BIN_DIR) $(TARGETS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# --- Tools ---

# 1. bdf2c (Header Generator)
$(BIN_DIR)/bdf2c: $(SRC_DIR)/bdf2c.c
	$(CC) $(CFLAGS) -o $@ $<

# 2. Header Generation (Depends on bdf2c)
$(LABEL_FONT_HEADER): $(BIN_DIR)/bdf2c
	@echo "Generating label_font.h from milkjf_8x16.bdf..."
	$(BIN_DIR)/bdf2c $(LABEL_FONT_SRC) > $@

# 3. gen_map (Depends on label_font.h)
$(BIN_DIR)/gen_map: $(SRC_DIR)/gen_map.c $(LABEL_FONT_HEADER) $(COMMON_HDRS)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/gen_map.c $(LDFLAGS)

$(BIN_DIR)/font_builder: $(SRC_DIR)/main.c $(SRC_DIR)/builder.c $(SRC_DIR)/builder.h $(SRC_DIR)/bdf_parser.c $(SRC_DIR)/bdf_parser.h $(COMMON_HDRS)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/main.c $(SRC_DIR)/builder.c $(SRC_DIR)/bdf_parser.c

$(BIN_DIR)/hex2bdf: $(SRC_DIR)/hex2bdf.c
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR)/viewer: src/viewer.c ../src/embfont16.c ../src/hal/posix/hal_posix.c $(COMMON_HDRS)
	$(CC) $(CFLAGS) -I../src -I../src/include src/viewer.c ../src/embfont16.c ../src/hal/posix/hal_posix.c -o $@

$(BIN_DIR)/bdf_recode: $(SRC_DIR)/bdf_recode.c
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR)/bdf_cut: $(SRC_DIR)/bdf_cut.c
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR)/bdf_diff: $(SRC_DIR)/bdf_diff.c
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR)/bdf_remap: $(SRC_DIR)/bdf_remap.c
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR)/bdf_jis2eucjp: $(SRC_DIR)/bdf_jis2eucjp.c
	$(CC) $(CFLAGS) -o $@ $<


clean:
	rm -rf $(BIN_DIR)
	rm -f $(LABEL_FONT_HEADER)

.PHONY: all clean
# Root Makefile

# Host compiler settings for the runtime library
CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra

# --- ディレクトリ設定 ---
TOOLS_DIR = tools
TOOLS_BIN = $(TOOLS_DIR)/bin
OUTPUT_DIR = output
WORK_DIR = $(OUTPUT_DIR)/work
MAPPING_DIR = mapping
LICENSE_DIR = assets/licenses
LIB_DIR = build
LIB = $(LIB_DIR)/libembfont16.a
LIB_SRCS = src/embfont16.c src/glyph_manupirate.c src/slice_manupirate.c
LIB_OBJS = $(LIB_SRCS:src/%.c=$(LIB_DIR)/%.o)

# --- ツールコマンド ---
CMD_RECODE  = $(TOOLS_BIN)/bdf_recode
CMD_GENMAP  = $(TOOLS_BIN)/gen_map
CMD_BUILDER = $(TOOLS_BIN)/font_builder
CMD_VIEWER  = $(TOOLS_BIN)/viewer
CMD_BDF_CUT  = $(TOOLS_BIN)/bdf_cut
CMD_BDF_REMAP = $(TOOLS_BIN)/bdf_remap
CMD_JIS2EUC = $(TOOLS_BIN)/bdf_jis2eucjp
CMD_UF2 = picotool uf2 convert

# ==========================================
#   CONFIGURATION: Font Sources & Sets
# ==========================================

# --- Output Sizes (Fixed) ---
# All ROM images are fixed at 4MB (0x400000 bytes).
SIZE_JP   = 0x400000
SIZE_FULL = 0x400000
SIZE_UNI  = 0x400000
SIZE_ANK  = 0x400000

# --- UF2 packaging ---
# RP2350 + 16MB flash: last 4MB (0x10000000 + 0x1000000 - 0x400000)
UF2_LOAD_ADDR = 0x10C00000
UF2_FAMILY_ID = 0xe48bff57


LICENSE_FILE_X11     = $(LICENSE_DIR)/CREDITS
LICENSE_FILE_UNIFONT = $(LICENSE_DIR)/LICENSE_UNIFONT

# --- 1. アセットソース定義 ---

# A. Japanese (Milkjf - Yaft)
ASSET_JP_8X16     = assets/yaft/fonts/milkjf/milkjf_8x16.bdf
ASSET_JP_K16      = assets/yaft/fonts/milkjf/milkjf_k16.bdf
ASSET_JP_8X16R    = assets/yaft/fonts/milkjf/milkjf_8x16r.bdf

# B. X11 Misc / Xorg Fonts (JIS, KR, CN)
ASSET_JIS_2004_1  = assets/misc/jiskan16-2004-1.bdf
ASSET_JIS_2000_2  = assets/misc/jiskan16-2000-2.bdf

ASSET_KR          = assets/xorg-font-daewoo/hanglm16.bdf
ASSET_CN          = assets/xorg-font-isas/gb16st.bdf

# C. Unifont (Universal Backup)
UNIFONT_VER       = 17.0.03
UNIFONT_URL_BASE  = https://unifoundry.com/pub/unifont/unifont-$(UNIFONT_VER)
UNIFONT_BUILD_URL = $(UNIFONT_URL_BASE)/font-builds
UNIFONT_FILE_GZ   = unifont_jp-$(UNIFONT_VER).bdf.gz
UNIFONT_ASSET_DIR = assets/unifont
ASSET_UNIFONT_GZ  = $(UNIFONT_ASSET_DIR)/$(UNIFONT_FILE_GZ)
ASSET_UNIFONT_BDF = $(UNIFONT_ASSET_DIR)/unifont_jp.bdf

# --- 2. 中間ファイル定義 (Processed BDFs) ---

# Milkjf
WORK_JP_K16_CUT_RAW = $(WORK_DIR)/milkjf_k16_jis_cut.bdf
WORK_JP_K16_UNI     = $(WORK_DIR)/milkjf_k16_uni.bdf
WORK_JP_8X16_FW     = $(WORK_DIR)/milkjf_8x16_fw.bdf
WORK_JP_8X16_CUT    = $(WORK_DIR)/milkjf_8x16_cut.bdf
WORK_JP_8X16R_UNI   = $(WORK_DIR)/milkjf_8x16r_uni.bdf

# JIS / X11
WORK_JIS_2004_1_U = $(WORK_DIR)/jiskan16-2004-1_uni.bdf

# JIS第2面用の中間ファイル
WORK_JIS_2000_2_SHIFT = $(WORK_DIR)/jiskan16-2000-2_shifted.bdf
WORK_JIS_2000_2_U     = $(WORK_DIR)/jiskan16-2000-2_uni.bdf

WORK_KR_UNI       = $(WORK_DIR)/hanglm16_uni.bdf
WORK_CN_UNI       = $(WORK_DIR)/gb16st_uni.bdf

WORK_KR_KANA_UNI  = $(WORK_DIR)/hanglm16_kana_uni.bdf
WORK_CN_KANA_UNI  = $(WORK_DIR)/gb16st_kana_uni.bdf

TTF_FILES = $(TARGET_FULL).ttf $(TARGET_JP).ttf $(TARGET_UNI).ttf $(TARGET_ANK).ttf

# --- 3. フォント構成セット定義 (Font Sets) ---

# [Set 1: EmbFont16 (Full)]
SET_FULL_LIST = \
	$(WORK_JP_8X16_CUT) \
	$(WORK_JP_K16_UNI) \
	$(WORK_JP_8X16R_UNI) \
	$(WORK_JIS_2004_1_U) \
	$(WORK_JIS_2000_2_U) \
	$(WORK_KR_UNI) \
	$(WORK_KR_KANA_UNI) \
	$(WORK_CN_UNI) \
	$(WORK_CN_KANA_UNI)

# [Set 2: EmbFont16JP]
SET_JP_LIST = \
	$(WORK_JP_8X16_CUT) \
	$(WORK_JP_K16_UNI) \
	$(WORK_JP_8X16R_UNI) \
	$(WORK_JIS_2004_1_U) \
	$(WORK_JIS_2000_2_U)

# [Set 3: EmbFont16Uni]
SET_UNI_LIST = \
	$(WORK_JP_8X16_CUT) \
	$(WORK_JP_K16_UNI) \
	$(WORK_JP_8X16R_UNI) \
	$(ASSET_UNIFONT_BDF) \
	$(WORK_JIS_2000_2_U)

SET_ANK_LIST = \
	$(WORK_JP_8X16_CUT)


# --- 4. 出力ターゲット定義 ---
TARGET_FULL = $(OUTPUT_DIR)/EmbFont16
TARGET_JP   = $(OUTPUT_DIR)/EmbFont16JP
TARGET_UNI  = $(OUTPUT_DIR)/EmbFont16Uni
TARGET_ANK  = $(OUTPUT_DIR)/EmbFont16Ank

UF2_FILES = $(TARGET_FULL).uf2 $(TARGET_JP).uf2 $(TARGET_UNI).uf2

# ==========================================
#   BUILD RULES
# ==========================================

all: directories tools lib roms

directories:
	@mkdir -p $(OUTPUT_DIR)
	@mkdir -p $(WORK_DIR)
	@mkdir -p $(MAPPING_DIR)
	@mkdir -p $(UNIFONT_ASSET_DIR)
	@mkdir -p $(LIB_DIR)

tools:
	@echo "=== Building Tools ==="
	$(MAKE) -C $(TOOLS_DIR)

# --- Runtime library ---
lib: $(LIB)

$(LIB): $(LIB_OBJS)
	@echo "=== Archiving $@ ==="
	ar rcs $@ $^

$(LIB_DIR)/%.o: src/%.c | directories
	$(CC) $(CFLAGS) -I src/include -c $< -o $@

# Rebuild host tools when their sources change. The sub-make decides what
# actually needs to be recompiled.
TOOL_DEPS = tools/Makefile \
            $(filter-out tools/src/label_font.h,$(wildcard tools/src/*.c) $(wildcard tools/src/*.h))
$(CMD_BUILDER) $(CMD_GENMAP) $(CMD_VIEWER): $(TOOL_DEPS)
	$(MAKE) -C $(TOOLS_DIR)

# --- Asset Processing Rules ---

# 1. Unifont
$(ASSET_UNIFONT_BDF): | directories
	@if [ ! -f $(ASSET_UNIFONT_GZ) ]; then \
		echo "=== Downloading Unifont JP $(UNIFONT_VER) ==="; \
		wget -O $(ASSET_UNIFONT_GZ) $(UNIFONT_BUILD_URL)/$(UNIFONT_FILE_GZ); \
	fi
	@echo "=== Extracting Unifont ==="
	gzip -d -c $(ASSET_UNIFONT_GZ) > $@

# 2. Milkjf K16
$(WORK_JP_K16_CUT_RAW): $(ASSET_JP_K16) | directories tools
	@echo "=== Cutting milkjf-k16 (JIS <= 0x2576) ==="
	$(CMD_BDF_CUT) -u $(ASSET_JP_K16) 0x3020 > $@

$(WORK_JP_K16_UNI): $(WORK_JP_K16_CUT_RAW) | directories tools
	@echo "=== Converting milkjf-k16-cut (JIS -> EUC -> Unicode) ==="
	$(CMD_JIS2EUC) $(WORK_JP_K16_CUT_RAW) -d $(WORK_DIR)
	# ファイル名は milkjf_k16_jis_cut_eucjp.bdf になります
	$(CMD_RECODE) $(WORK_DIR)/milkjf_k16_jis_cut_eucjp.bdf EUCJP-WIN $@

# 3. Milkjf 8x16
$(WORK_JP_8X16_FW): $(ASSET_JP_8X16) | directories tools
	@echo "=== Cutting milkjf-8x16 (>= 0x20) ==="
	$(CMD_BDF_CUT) $(ASSET_JP_8X16) 0x20 > $@

$(WORK_JP_8X16_CUT): $(WORK_JP_8X16_FW) | directories tools
	@echo "=== Cutting milkjf-8x16 (<= 0x7E) ==="
	$(CMD_BDF_CUT) -u $(WORK_JP_8X16_FW) 0x7E > $@

$(WORK_JP_8X16R_UNI): $(ASSET_JP_8X16R) | directories tools
	@echo "=== Remapping milkjf-8x16r (Kana/Hira -> Unicode/PUA) ==="
	$(CMD_BDF_REMAP) $(ASSET_JP_8X16R) $@ 0xA1-0xDF:0xFF61 0x86-0x9F:0xF766 0xE0-0xFD:0xF780

# 4. Other X11 Fonts
$(WORK_JIS_2004_1_U): $(ASSET_JIS_2004_1) | directories tools
	@echo "=== Converting jiskan16-2004-1 (EUC-JP -> Unicode) ==="
	$(CMD_RECODE) $(ASSET_JIS_2004_1) EUCJP-WIN $@

# JIS X 0213 plane 2
$(WORK_JIS_2000_2_U): $(ASSET_JIS_2000_2) | directories tools
	@echo "=== Converting jiskan16-2000-2 (JIS -> EUC-JP Plane 2 -> Unicode) ==="
	# 1. JISエンコーディングをEUC-JP領域へシフト (+0x8F8080)
	$(CMD_JIS2EUC) $(ASSET_JIS_2000_2) -d $(WORK_DIR)
	# 2. 変換されたファイルをUnicodeへリコード
	$(CMD_RECODE) $(WORK_DIR)/jiskan16-2000-2_eucjp.bdf EUC-JISX0213 $@

$(WORK_KR_UNI): $(ASSET_KR) | directories tools
	@echo "=== Converting Korean Font (KSC5601 -> EUC-KR -> Unicode) ==="
	$(CMD_JIS2EUC) $(ASSET_KR) -d $(WORK_DIR)
	# ファイル名は hanglm16_eucjp.bdf になります
	$(CMD_RECODE) $(WORK_DIR)/hanglm16_eucjp.bdf EUC-KR $@


$(WORK_CN_UNI): $(ASSET_CN) | directories tools
	@echo "=== Converting Chinese Font (GB2312 -> EUC-CN -> Unicode) ==="
	$(CMD_JIS2EUC) $(ASSET_CN) -d $(WORK_DIR)
	# ファイル名は gb16st_eucjp.bdf になります
	$(CMD_RECODE) $(WORK_DIR)/gb16st_eucjp.bdf GB2312 $@

# --- Kana remapping for Korean/Chinese ---
$(WORK_KR_KANA_UNI): $(WORK_KR_UNI) | directories tools
	@echo "=== Remapping Korean Kana to PUA (FULL) ==="
	$(CMD_BDF_REMAP) $< $@ 0x3041-0x3093:0xF5E1 0x30A1-0x30F6:0xF641

$(WORK_CN_KANA_UNI): $(WORK_CN_UNI) | directories tools
	@echo "=== Remapping Chinese Kana to PUA (FULL) ==="
	$(CMD_BDF_REMAP) $< $@ 0x3041-0x3093:0xF6A1 0x30A1-0x30F6:0xF701



# --- Merge & ROM Generation ---

roms: $(TARGET_FULL).bin $(TARGET_JP).bin $(TARGET_UNI).bin

# 1. Full (Embed CREDITS + Fixed Size 4MB)
$(TARGET_FULL).bdf: $(SET_FULL_LIST) $(CMD_GENMAP) | directories tools
	@echo "=== Generating Map & Merging [FULL] ==="
	$(CMD_GENMAP) $(SET_FULL_LIST) --bdf $@ --png $(OUTPUT_DIR)/charts_full

$(TARGET_FULL).bin: $(TARGET_FULL).bdf $(CMD_BUILDER) | tools
	@echo "=== Building ROM [FULL] (Size: $(SIZE_FULL)) ==="
	# -l でライセンス埋め込み, -s でサイズ固定
	$(CMD_BUILDER) $< $@ -l $(LICENSE_FILE_X11) -s $(SIZE_FULL)
	@echo "=== Copying License Text ==="
	cp $(LICENSE_FILE_X11) $(OUTPUT_DIR)/CREDITS.txt

# 2. JP Only (No License + Fixed Size 4MB)
$(TARGET_JP).bdf: $(SET_JP_LIST) $(CMD_GENMAP) | directories tools
	@echo "=== Generating Map & Merging [JP] ==="
	$(CMD_GENMAP) $(SET_JP_LIST) --bdf $@ --png $(OUTPUT_DIR)/charts_jp

$(TARGET_JP).bin: $(TARGET_JP).bdf $(CMD_BUILDER) | tools
	@echo "=== Building ROM [JP] (Size: $(SIZE_JP)) ==="
	# ライセンスなし, -s でサイズ固定
	$(CMD_BUILDER) $< $@ -s $(SIZE_JP)

# 3. Uni (Embed LICENSE_UNIFONT + Fixed Size 4MB)
$(TARGET_UNI).bdf: $(SET_UNI_LIST) $(CMD_GENMAP) | directories tools
	@echo "=== Generating Map & Merging [UNI] ==="
	$(CMD_GENMAP) $(SET_UNI_LIST) --bdf $@ --png $(OUTPUT_DIR)/charts_uni

$(TARGET_UNI).bin: $(TARGET_UNI).bdf $(CMD_BUILDER) | tools
	@echo "=== Building ROM [UNI] (Size: $(SIZE_UNI)) ==="
	# -l でライセンス埋め込み, -s でサイズ固定
	$(CMD_BUILDER) $< $@ -l $(LICENSE_FILE_UNIFONT) -s $(SIZE_UNI)
	@echo "=== Copying License Text ==="
	cp $(LICENSE_FILE_UNIFONT) $(OUTPUT_DIR)/LICENSE_UNIFONT.txt


# 4. ANK only
.PHONY: ank
ank: $(TARGET_ANK).bin

$(TARGET_ANK).bdf: $(SET_ANK_LIST) $(CMD_GENMAP) | directories tools
	@echo "=== Generating Map & Merging [ANK] ==="
	$(CMD_GENMAP) $(SET_ANK_LIST) --bdf $@ --png $(OUTPUT_DIR)/charts_ank

$(TARGET_ANK).bin: $(TARGET_ANK).bdf $(CMD_BUILDER) | tools
	@echo "=== Building ROM [ANK] (Size: $(SIZE_ANK)) ==="
	$(CMD_BUILDER) $< $@ -s $(SIZE_ANK)


# 5. UF2 (map the 4MB ROM image to the last 4MB of 16MB flash)
.PHONY: uf2
uf2: $(UF2_FILES)

$(OUTPUT_DIR)/%.uf2: $(OUTPUT_DIR)/%.bin
	@echo "=== Packaging UF2 [$(notdir $*)] at $(UF2_LOAD_ADDR) ==="
	$(CMD_UF2) $< $@ -o $(UF2_LOAD_ADDR) --family $(UF2_FAMILY_ID) --platform rp2350



# TTF 生成用ターゲット
.PHONY: ttf
ttf: $(TTF_FILES)

output/%.ttf: output/%.bdf
	@echo "Generating TTF from $<..."
	@echo 'Open($$1);' > $(WORK_DIR)/ttf_conv.pe
	@echo 'SelectAll();' >> $(WORK_DIR)/ttf_conv.pe
	@echo 'Import($$1);' >> $(WORK_DIR)/ttf_conv.pe
	@echo 'AutoTrace();' >> $(WORK_DIR)/ttf_conv.pe
	@echo 'SetFontOrder(2);' >> $(WORK_DIR)/ttf_conv.pe
	@echo 'ScaleToEm(1024);' >> $(WORK_DIR)/ttf_conv.pe
	@echo 'RoundToInt();' >> $(WORK_DIR)/ttf_conv.pe
	@echo 'SetFontNames("EmbFont16", "EmbFont16", "EmbFont16", "Regular", "");' >> $(WORK_DIR)/ttf_conv.pe
	@echo 'Generate($$1:r + ".ttf", "ttf");' >> $(WORK_DIR)/ttf_conv.pe
	@echo 'Close();' >> $(WORK_DIR)/ttf_conv.pe
	fontforge -script $(WORK_DIR)/ttf_conv.pe $<


check: roms ank $(CMD_VIEWER)
	@echo "--- Checking EmbFont16 (Full) ---"
	$(CMD_VIEWER) $(TARGET_FULL).bin 41 3042
	@echo "--- Checking EmbFont16JP (JP Only) ---"
	$(CMD_VIEWER) $(TARGET_JP).bin 41 3042
	@echo "--- Checking EmbFont16Uni (Milkjf+Uni) ---"
	$(CMD_VIEWER) $(TARGET_UNI).bin 41 3042
	@echo "--- Checking EmbFont16Ank ---"
	$(CMD_VIEWER) $(TARGET_ANK).bin 41 3042

clean:
	rm -rf $(OUTPUT_DIR) $(MAPPING_DIR) $(LIB_DIR)
	rm -f $(ASSET_UNIFONT_BDF)
	$(MAKE) -C $(TOOLS_DIR) clean

.PHONY: all directories tools lib roms ank uf2 ttf check clean


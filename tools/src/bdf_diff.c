#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_GLYPHS 25000  // 少し余裕を持たせました
#define MAX_LINE 1024
#define MAX_BITMAP_LINES 128

typedef struct {
    int encoding;
    char name[128];
    // ヘッダ情報（BBX, DWIDTHなど比較用）
    char dwidth[128];
    char bbx[128];
    // ビットマップデータ
    char bitmap[MAX_BITMAP_LINES][128];
    int bitmap_count;
    // 生のデータブロック（再出力用）
    char *raw_data; 
} Glyph;

typedef struct {
    char filename[256];
    char header[MAX_GLYPHS][128];
    int header_lines;
    Glyph glyphs[MAX_GLYPHS];
    int count;
} Font;

// 前方宣言
void load_bdf(const char *path, Font *font);
void write_bdf(const char *path, Font *base_font, int *indices, int count);
int compare_glyphs(Glyph *g1, Glyph *g2);
void get_filename_without_ext(const char *path, char *out);

int main(int argc, char *argv[]) {
    char *path1 = NULL;
    char *path2 = NULL;
    char *out_dir = ".";

    // 引数解析
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dir") == 0 && i + 1 < argc) {
            out_dir = argv[++i];
        } else if (!path1) {
            path1 = argv[i];
        } else if (!path2) {
            path2 = argv[i];
        }
    }

    if (!path1 || !path2) {
        fprintf(stderr, "Usage: %s <font1.bdf> <font2.bdf> [--dir output_dir]\n", argv[0]);
        return 1;
    }

    // ディレクトリ作成
    struct stat st = {0};
    if (stat(out_dir, &st) == -1) {
#ifdef _WIN32
        mkdir(out_dir);
#else
        mkdir(out_dir, 0755);
#endif
    }

    // ヒープ領域に巨大構造体を確保
    Font *f1 = (Font *)calloc(1, sizeof(Font));
    Font *f2 = (Font *)calloc(1, sizeof(Font));
    if (!f1 || !f2) {
        fprintf(stderr, "Memory allocation failed (Font struct is too large)\n");
        return 1;
    }

    // フォント読み込み
    printf("Loading %s...\n", path1);
    load_bdf(path1, f1);
    printf("Loading %s...\n", path2);
    load_bdf(path2, f2);

    // インデックスリスト用のメモリ確保（スタックオーバーフロー回避）
    int *common_indices = (int *)calloc(MAX_GLYPHS, sizeof(int));
    int common_count = 0;

    int *diff_indices_f1 = (int *)calloc(MAX_GLYPHS, sizeof(int));
    int diff_count_f1 = 0;

    int *diff_indices_f2 = (int *)calloc(MAX_GLYPHS, sizeof(int));
    int diff_count_f2 = 0;

    if (!common_indices || !diff_indices_f1 || !diff_indices_f2) {
        fprintf(stderr, "Memory allocation for indices failed\n");
        return 1;
    }

    // ---------------------------------------------------------
    // 比較ロジック
    // ---------------------------------------------------------

    // 1. F1を走査し、F2との比較を行う
    for (int i = 0; i < f1->count; i++) {
        Glyph *g1 = &f1->glyphs[i];
        int found_in_f2 = -1;

        // F2から同じエンコーディングを探す
        for (int j = 0; j < f2->count; j++) {
            if (f2->glyphs[j].encoding == g1->encoding) {
                found_in_f2 = j;
                break;
            }
        }

        if (found_in_f2 != -1) {
            // コードポイントが一致する文字がF2にある
            Glyph *g2 = &f2->glyphs[found_in_f2];
            
            if (compare_glyphs(g1, g2)) {
                // 字形も完全に一致 -> 共通ファイルへ (F1からもF2からも差分としては出力しない)
                common_indices[common_count++] = i;
            } else {
                // 字形が違う -> F1側の差分として登録
                // (F2側の差分は、後のF2走査ループで登録される)
                diff_indices_f1[diff_count_f1++] = i;
            }
        } else {
            // F2に存在しない -> F1独自の文字
            diff_indices_f1[diff_count_f1++] = i;
        }
    }

    // 2. F2を走査し、F1との比較を行う（F2側の差分抽出用）
    for (int i = 0; i < f2->count; i++) {
        Glyph *g2 = &f2->glyphs[i];
        int found_in_f1 = -1;

        for (int j = 0; j < f1->count; j++) {
            if (f1->glyphs[j].encoding == g2->encoding) {
                found_in_f1 = j;
                break;
            }
        }

        if (found_in_f1 != -1) {
            // コードポイントが一致する文字がF1にある
            Glyph *g1 = &f1->glyphs[found_in_f1];

            if (compare_glyphs(g1, g2)) {
                // 完全一致 -> 既にF1ループで共通リストに入れているので何もしない
            } else {
                // 字形が違う -> F2側の差分として登録
                diff_indices_f2[diff_count_f2++] = i;
            }
        } else {
            // F1に存在しない -> F2独自の文字
            diff_indices_f2[diff_count_f2++] = i;
        }
    }

    // ---------------------------------------------------------
    // 出力処理
    // ---------------------------------------------------------
    
    char name1[128], name2[128];
    get_filename_without_ext(path1, name1);
    get_filename_without_ext(path2, name2);

    char out_path1[512], out_path2[512], out_common[512];
    sprintf(out_path1, "%s/%s-diff.bdf", out_dir, name1);
    sprintf(out_path2, "%s/%s-diff.bdf", out_dir, name2);
    sprintf(out_common, "%s/font-diff.bdf", out_dir); 

    // 出力 6: F1の独自 or 形状不一致
    printf("Writing %s (Count: %d)...\n", out_path1, diff_count_f1);
    write_bdf(out_path1, f1, diff_indices_f1, diff_count_f1);

    // 出力 7: F2の独自 or 形状不一致
    printf("Writing %s (Count: %d)...\n", out_path2, diff_count_f2);
    write_bdf(out_path2, f2, diff_indices_f2, diff_count_f2);

    // 出力: 共通かつ完全一致
    printf("Writing %s (Identical Count: %d)...\n", out_common, common_count);
    write_bdf(out_common, f1, common_indices, common_count);

    // メモリ解放
    for(int i=0; i<f1->count; i++) free(f1->glyphs[i].raw_data);
    for(int i=0; i<f2->count; i++) free(f2->glyphs[i].raw_data);
    free(f1);
    free(f2);
    free(common_indices);
    free(diff_indices_f1);
    free(diff_indices_f2);

    return 0;
}

// ---------------- 以下、変更なしの補助関数 ----------------

void load_bdf(const char *path, Font *font) {
    FILE *fp = fopen(path, "r");
    if (!fp) { perror("fopen"); exit(1); }

    char line[MAX_LINE];
    int in_header = 1;
    Glyph *current = NULL;
    
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;

        if (strncmp(line, "STARTCHAR", 9) == 0) {
            in_header = 0;
            if (font->count >= MAX_GLYPHS) break;
            current = &font->glyphs[font->count];
            strcpy(current->name, line + 10);
            current->bitmap_count = 0;
            current->raw_data = calloc(1, 4096); 
            strcpy(current->raw_data, line);
            strcat(current->raw_data, "\n");
        } else if (in_header) {
            strcpy(font->header[font->header_lines++], line);
        } else if (current) {
            strcat(current->raw_data, line);
            strcat(current->raw_data, "\n");

            if (strncmp(line, "ENCODING", 8) == 0) {
                current->encoding = atoi(line + 9);
            } else if (strncmp(line, "DWIDTH", 6) == 0) {
                strcpy(current->dwidth, line);
            } else if (strncmp(line, "BBX", 3) == 0) {
                strcpy(current->bbx, line);
            } else if (strncmp(line, "BITMAP", 6) == 0) {
                while (fgets(line, sizeof(line), fp)) {
                    line[strcspn(line, "\r\n")] = 0;
                    strcat(current->raw_data, line);
                    strcat(current->raw_data, "\n");
                    if (strncmp(line, "ENDCHAR", 7) == 0) {
                        font->count++;
                        current = NULL;
                        break;
                    }
                    if (current->bitmap_count < MAX_BITMAP_LINES) {
                        strcpy(current->bitmap[current->bitmap_count++], line);
                    }
                }
            }
        }
    }
    fclose(fp);
}

void write_bdf(const char *path, Font *base_font, int *indices, int count) {
    FILE *fp = fopen(path, "w");
    if (!fp) { perror("fopen write"); exit(1); }

    for (int i = 0; i < base_font->header_lines; i++) {
        if (strncmp(base_font->header[i], "CHARS", 5) == 0) {
            fprintf(fp, "CHARS %d\n", count);
        } else {
            fprintf(fp, "%s\n", base_font->header[i]);
        }
    }

    for (int i = 0; i < count; i++) {
        Glyph *g = &base_font->glyphs[indices[i]];
        fprintf(fp, "%s", g->raw_data);
    }
    
    fprintf(fp, "ENDFONT\n");
    fclose(fp);
}

int compare_glyphs(Glyph *g1, Glyph *g2) {
    if (g1->encoding != g2->encoding) return 0;
    if (g1->bitmap_count != g2->bitmap_count) return 0;
    if (strcmp(g1->bbx, g2->bbx) != 0) return 0;
    if (strcmp(g1->dwidth, g2->dwidth) != 0) return 0;
    
    for (int i = 0; i < g1->bitmap_count; i++) {
        if (strcmp(g1->bitmap[i], g2->bitmap[i]) != 0) return 0;
    }
    return 1;
}

void get_filename_without_ext(const char *path, char *out) {
    char temp[256];
    strcpy(temp, path);
    char *base = basename(temp);
    char *dot = strrchr(base, '.');
    if (dot) *dot = '\0';
    strcpy(out, base);
}
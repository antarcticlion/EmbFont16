#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <stdbool.h>

void process_jis2euc(const char *input_path, const char *output_dir) {
    FILE *in = fopen(input_path, "r");
    if (!in) { perror("Failed to open input"); return; }

    char path_copy[1024];
    strncpy(path_copy, input_path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy)-1] = '\0';
	// tools/src/bdf_jis2eucjp.c の main 関数直前のファイル名生成部分を修正
	char *base = basename(path_copy);
	char output_path[2048];
	char base_name[1024];

	strncpy(base_name, base, sizeof(base_name)-1);
	char *dot = strrchr(base_name, '.');
	if (dot) *dot = '\0'; // 拡張子を取り除く

	if (output_dir) {
	    snprintf(output_path, sizeof(output_path), "%s/%s_eucjp.bdf", output_dir, base_name);
	} else {
	    snprintf(output_path, sizeof(output_path), "%s_eucjp.bdf", base_name);
	}

    FILE *out = fopen(output_path, "w");
    if (!out) { perror("Failed to open output"); fclose(in); return; }

    char line[1024];
    uint32_t current_jis_code = -1;
    bool has_encoding_glyph = false;
    bool is_jis_registry = false;
    bool is_2nd_plane = false;
    bool is_jis_men_ku_ten = false;



	while (fgets(line, sizeof(line), in)) {
		if (strncmp(line, "CHARSET_REGISTRY", 16) == 0) {
		    // 日・中・韓の主要な GL プレーン規格を網羅
		    if (strstr(line, "JISX0213")){
		        is_jis_registry = true;
		        is_jis_men_ku_ten = true;
		    }
		    if (strstr(line, "JISX0208") || 
		        strstr(line, "GB2312")   || strstr(line, "KSC5601")) {
		        is_jis_registry = true;
		    }
		    fputs(line, out);
		} else if ( !strncmp(line, "CHARSET_ENCODING", 16)  ){
		    // 該当する規格で、かつ Encoding が "0" なら "1" (EUC) へ書き換え
		    if (is_jis_registry && strstr(line, "\"0\"")) {
		        fprintf(out, "CHARSET_ENCODING \"1\"\n");
		    } else if (is_jis_registry && strstr(line, "\"2\"")) {
		        fprintf(out, "CHARSET_ENCODING \"1\"\n");
		        is_2nd_plane = true;
		    } else {
		        fputs(line, out);
		    }
		} else if (strncmp(line, "STARTCHAR", 9) == 0) {
            char name[256];
            if(is_jis_men_ku_ten){
	            sscanf(line + 10, "%s", name);
	            if((uint32_t)strtol(name, NULL, 16) == 2){
					//2面
					is_2nd_plane = 1;
				}else{
					//1面
					is_2nd_plane = 0;
				}
	            sscanf(line + 13, "%s", name);
	            current_jis_code  = (strtol(name, NULL, 16) + 0x20) << 8;
	            sscanf(line + 16, "%s", name);
	            current_jis_code += (uint32_t)strtol(name, NULL, 16) + 0x20;
			}else{
	            sscanf(line + 10, "%s", name);
	            current_jis_code = (uint32_t)strtol(name, NULL, 16);
	        }
            if(is_2nd_plane){
	            fprintf(out, "STARTCHAR %06X\n", (uint32_t)(current_jis_code + 0x8F8080));
            }else{
    	        fprintf(out, "STARTCHAR %04X\n", (uint16_t)(current_jis_code + 0x8080));
            }
            has_encoding_glyph = false;
        } else if (strncmp(line, "ENCODING", 8) == 0) {
            int enc = atoi(line + 9);
            if(is_2nd_plane){
	            fprintf(out, "ENCODING %u\n", (unsigned)(enc + 0x8F8080));
	        }else{
	            fprintf(out, "ENCODING %u\n", (unsigned)(enc + 0x8080));
			}
            has_encoding_glyph = true;
        } else if (strncmp(line, "BITMAP", 6) == 0) {
            if (!has_encoding_glyph && current_jis_code != -1) {
	            if(is_2nd_plane){
		            fprintf(out, "ENCODING %u\n", (unsigned)(current_jis_code + 0x8F8080));
		        }else{
		            fprintf(out, "ENCODING %u\n", (unsigned)(current_jis_code + 0x8080));
				}
            }
            fputs(line, out);
        } else {
            fputs(line, out);
        }
    }
    fclose(in);
    fclose(out);
}

int main(int argc, char *argv[]) {
    char *output_dir = NULL;
    char *input_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            output_dir = argv[++i];
        } else {
            input_file = argv[i];
        }
    }

    if (!input_file) {
        fprintf(stderr, "Usage: %s <input.bdf> [-d output_dir]\n", argv[0]);
        return 1;
    }

    process_jis2euc(input_file, output_dir);
    return 0;
}
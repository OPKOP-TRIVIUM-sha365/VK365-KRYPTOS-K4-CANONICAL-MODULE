#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BIT_LEN 24

typedef struct {
    char id[32];
    char timestamp[32];
    char berlin[32];
    int rgb[3];
    char hex[7];
} KatVector;

static char *read_text_file(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) return NULL;
    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return NULL; }
    long size = ftell(fp);
    if (size < 0) { fclose(fp); return NULL; }
    rewind(fp);
    char *buf = (char *)malloc((size_t)size + 1);
    if (buf == NULL) { fclose(fp); return NULL; }
    size_t n = fread(buf, 1, (size_t)size, fp);
    buf[n] = '\0';
    fclose(fp);
    return buf;
}

static int starts_with_str(const char *text, const char *needle) {
    size_t n = strlen(needle);
    return strncmp(text, needle, n) == 0;
}

static void trim_ws(char *text) {
    while (*text == ' ' || *text == '\n' || *text == '\r' || *text == '\t') {
        memmove(text, text + 1, strlen(text));
    }
}

static int extract_json_string_value(const char *obj, const char *key, char *out, size_t out_size) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *pos = strstr(obj, pattern);
    if (pos == NULL) return 0;
    pos = strchr(pos, ':');
    if (pos == NULL) return 0;
    pos++;
    while (*pos == ' ' || *pos == '\n' || *pos == '\r' || *pos == '\t') pos++;
    if (*pos != '"') return 0;
    pos++;
    size_t i = 0;
    while (*pos != '\0' && *pos != '"' && i + 1 < out_size) {
        out[i++] = *pos++;
    }
    out[i] = '\0';
    return i > 0;
}

static int extract_json_int_array(const char *obj, const char *key, int out[3]) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *pos = strstr(obj, pattern);
    if (pos == NULL) return 0;
    pos = strchr(pos, '[');
    if (pos == NULL) return 0;
    int val[3] = {0,0,0};
    if (sscanf(pos, "[%d, %d, %d]", &val[0], &val[1], &val[2]) != 3) {
        return 0;
    }
    out[0] = val[0]; out[1] = val[1]; out[2] = val[2];
    return 1;
}

static int object_has_key(const char *obj, const char *key) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    return strstr(obj, pattern) != NULL;
}

static int duplicate_id_exists(const KatVector *items, int count, const char *id) {
    for (int i = 0; i < count; ++i) {
        if (strcmp(items[i].id, id) == 0) return 1;
    }
    return 0;
}

static int parse_kat_vectors(const char *json_text, KatVector *out, int max_vectors) {
    const char *vectors_pos = strstr(json_text, "\"vectors\"");
    if (vectors_pos == NULL) return 0;
    const char *array_start = strchr(vectors_pos, '[');
    if (array_start == NULL) return 0;

    const char *cursor = array_start + 1;
    int count = 0;
    while (count < max_vectors && *cursor != '\0') {
        while (*cursor == ' ' || *cursor == '\n' || *cursor == '\r' || *cursor == '\t') cursor++;
        if (*cursor == ']') break;
        if (*cursor != '{') {
            cursor++;
            continue;
        }

        const char *object_start = cursor;
        int depth = 0;
        while (*cursor != '\0') {
            if (*cursor == '{') depth++;
            if (*cursor == '}') {
                depth--;
                if (depth == 0) break;
            }
            cursor++;
        }
        if (*cursor == '\0') break;

        size_t object_len = (size_t)(cursor - object_start + 1);
        char *obj = (char *)malloc(object_len + 1);
        if (obj == NULL) return 0;
        memcpy(obj, object_start, object_len);
        obj[object_len] = '\0';

        KatVector kv;
        memset(&kv, 0, sizeof(kv));
        if (!object_has_key(obj, "id") || !object_has_key(obj, "timestamp") || !object_has_key(obj, "berlin") || !object_has_key(obj, "rgb") || !object_has_key(obj, "hex")) {
            free(obj);
            return 0;
        }

        if (!extract_json_string_value(obj, "id", kv.id, sizeof(kv.id)) ||
            !extract_json_string_value(obj, "timestamp", kv.timestamp, sizeof(kv.timestamp)) ||
            !extract_json_string_value(obj, "berlin", kv.berlin, sizeof(kv.berlin)) ||
            !extract_json_string_value(obj, "hex", kv.hex, sizeof(kv.hex)) ||
            !extract_json_int_array(obj, "rgb", kv.rgb)) {
            free(obj);
            return 0;
        }

        if (duplicate_id_exists(out, count, kv.id)) {
            free(obj);
            return 0;
        }

        out[count++] = kv;
        free(obj);
        cursor++;
    }
    return count;
}

static int validate_hex_string(const char *hex) {
    if (hex == NULL || strlen(hex) != 6) return 0;
    for (size_t i = 0; i < strlen(hex); ++i) {
        unsigned char c = (unsigned char)hex[i];
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) return 0;
    }
    return 1;
}

static int is_valid_bitstream(const char *bits) {
    if (bits == NULL || strlen(bits) != BIT_LEN) return 0;
    for (size_t i = 0; i < strlen(bits); ++i) {
        if (bits[i] != '0' && bits[i] != '1') return 0;
    }
    return 1;
}

static void test_kat_vectors(void) {
    char *json_text = read_text_file("../vectors/bu_kat_001.json");
    if (json_text == NULL) {
        fprintf(stderr, "Unable to read KAT JSON\n");
        exit(1);
    }

    KatVector vectors[16];
    int count = parse_kat_vectors(json_text, vectors, 16);
    free(json_text);
    if (count != 5) {
        fprintf(stderr, "Expected 5 KAT vectors, found %d\n", count);
        exit(1);
    }

    for (int i = 0; i < count; ++i) {
        if (!is_valid_bitstream(vectors[i].berlin)) {
            fprintf(stderr, "Invalid bitstream: %s\n", vectors[i].berlin);
            exit(1);
        }
        if (!validate_hex_string(vectors[i].hex)) {
            fprintf(stderr, "Invalid hex: %s\n", vectors[i].hex);
            exit(1);
        }
        for (int j = 0; j < 3; ++j) {
            if (vectors[i].rgb[j] < 0 || vectors[i].rgb[j] > 255) {
                fprintf(stderr, "RGB out of range\n");
                exit(1);
            }
        }
    }
}

static void test_duplicate_kat_id_negative(void) {
    KatVector items[2];
    strcpy(items[0].id, "DUP-1");
    strcpy(items[1].id, "DUP-1");
    if (!duplicate_id_exists(items, 2, "DUP-1")) {
        fprintf(stderr, "Duplicate ID detection failed\n");
        exit(1);
    }
}

int main(void) {
    test_kat_vectors();
    test_duplicate_kat_id_negative();
    printf("PASS: KAT corpus integrity and strict validation checks passed.\n");
    return 0;
}

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "third_party/cJSON.h"

#define KAT_VECTOR_COUNT 5
#define BITSTREAM_LEN 24
#define HEX_LEN 6

typedef struct {
    char id[32];
    char timestamp[32];
    char berlin[32];
    int rgb[3];
    char hex[7];
} KATVector;

static bool is_ascii_digit_char(char ch) {
    return ch >= '0' && ch <= '9';
}

static bool is_ascii_upper_hex_char(char ch) {
    return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F');
}

static bool is_valid_bitstream(const char *bits) {
    if (bits == NULL) {
        return false;
    }
    size_t len = strlen(bits);
    if (len != BITSTREAM_LEN) {
        return false;
    }
    for (size_t i = 0; i < len; ++i) {
        if (bits[i] != '0' && bits[i] != '1') {
            return false;
        }
    }
    return true;
}

static bool is_valid_hex_6_upper(const char *hex) {
    if (hex == NULL) {
        return false;
    }
    if (strlen(hex) != HEX_LEN) {
        return false;
    }
    for (size_t i = 0; i < strlen(hex); ++i) {
        if (!is_ascii_upper_hex_char(hex[i])) {
            return false;
        }
    }
    return true;
}

static bool validate_timestamp_ascii(const char *ts) {
    if (ts == NULL) {
        return false;
    }
    if (strlen(ts) != 20) {
        return false;
    }

    // YYYY-MM-DDTHH:MM:SSZ
    if (ts[4] != '-' || ts[7] != '-' || ts[10] != 'T' || ts[13] != ':' || ts[16] != ':' || ts[19] != 'Z') {
        return false;
    }

    for (int i = 0; i < 20; ++i) {
        if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16) {
            continue;
        }
        if (i == 19) {
            if (ts[i] != 'Z') return false;
            continue;
        }
        if (!is_ascii_digit_char(ts[i])) {
            return false;
        }
    }

    int year = (ts[0] - '0') * 1000 + (ts[1] - '0') * 100 + (ts[2] - '0') * 10 + (ts[3] - '0');
    int mon = (ts[5] - '0') * 10 + (ts[6] - '0');
    int day = (ts[8] - '0') * 10 + (ts[9] - '0');
    int hour = (ts[11] - '0') * 10 + (ts[12] - '0');
    int min = (ts[14] - '0') * 10 + (ts[15] - '0');
    int sec = (ts[17] - '0') * 10 + (ts[18] - '0');

    if (year < 1 || year > 9999) return false;
    if (mon < 1 || mon > 12) return false;
    if (day < 1 || day > 31) return false;
    if (hour < 0 || hour > 23) return false;
    if (min < 0 || min > 59) return false;
    if (sec < 0 || sec > 59) return false;

    // Basic Gregorian sanity with leap-year aware February.
    if (mon == 2) {
        bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        if (leap) {
            if (day > 29) return false;
        } else {
            if (day > 28) return false;
        }
    } else if (mon == 4 || mon == 6 || mon == 9 || mon == 11) {
        if (day > 30) return false;
    } else if (mon == 1 || mon == 3 || mon == 5 || mon == 7 || mon == 8 || mon == 10 || mon == 12) {
        if (day > 31) return false;
    }

    return true;
}

static bool validate_vector_object(cJSON *item) {
    cJSON *id_obj = cJSON_GetObjectItemCaseSensitive(item, "id");
    cJSON *ts_obj = cJSON_GetObjectItemCaseSensitive(item, "timestamp");
    cJSON *berlin_obj = cJSON_GetObjectItemCaseSensitive(item, "berlin");
    cJSON *rgb_obj = cJSON_GetObjectItemCaseSensitive(item, "rgb");
    cJSON *hex_obj = cJSON_GetObjectItemCaseSensitive(item, "hex");

    if (!cJSON_IsString(id_obj) ||
        !cJSON_IsString(ts_obj) ||
        !cJSON_IsString(berlin_obj) ||
        !cJSON_IsArray(rgb_obj) ||
        !cJSON_IsString(hex_obj)) {
        return false;
    }

    if (!validate_timestamp_ascii(ts_obj->valuestring)) {
        return false;
    }
    if (!is_valid_bitstream(berlin_obj->valuestring)) {
        return false;
    }
    if (!is_valid_hex_6_upper(hex_obj->valuestring)) {
        return false;
    }

    if (cJSON_GetArraySize(rgb_obj) != 3) {
        return false;
    }
    for (int i = 0; i < 3; ++i) {
        cJSON *entry = cJSON_GetArrayItem(rgb_obj, i);
        if (!cJSON_IsNumber(entry)) {
            return false;
        }
        double val = entry->valuedouble;
        if (val < 0.0 || val > 255.0 || val != (int)val) {
            return false;
        }
    }

    return true;
}

static bool duplicate_ids_exist(const cJSON *vectors) {
    int count = cJSON_GetArraySize((cJSON *)vectors);
    for (int i = 0; i < count; ++i) {
        cJSON *item_a = cJSON_GetArrayItem((cJSON *)vectors, i);
        cJSON *id_a = cJSON_GetObjectItemCaseSensitive(item_a, "id");
        if (!cJSON_IsString(id_a)) {
            return true;
        }
        for (int j = i + 1; j < count; ++j) {
            cJSON *item_b = cJSON_GetArrayItem((cJSON *)vectors, j);
            cJSON *id_b = cJSON_GetObjectItemCaseSensitive(item_b, "id");
            if (!cJSON_IsString(id_b)) {
                return true;
            }
            if (strcmp(id_a->valuestring, id_b->valuestring) == 0) {
                return true;
            }
        }
    }
    return false;
}

static bool parse_and_validate_corpus(const char *json_text) {
    cJSON *root = cJSON_Parse(json_text);
    if (root == NULL) {
        fprintf(stderr, "Invalid JSON: parse failed\n");
        return false;
    }

    cJSON *vectors = cJSON_GetObjectItemCaseSensitive(root, "vectors");
    if (vectors == NULL || !cJSON_IsArray(vectors)) {
        fprintf(stderr, "Invalid JSON: vectors must be an array\n");
        cJSON_Delete(root);
        return false;
    }

    if (cJSON_GetArraySize(vectors) != KAT_VECTOR_COUNT) {
        fprintf(stderr, "Invalid corpus: expected %d vectors, got %d\n", KAT_VECTOR_COUNT, cJSON_GetArraySize(vectors));
        cJSON_Delete(root);
        return false;
    }

    for (int i = 0; i < cJSON_GetArraySize(vectors); ++i) {
        cJSON *item = cJSON_GetArrayItem(vectors, i);
        if (!cJSON_IsObject(item)) {
            fprintf(stderr, "Invalid vector: element %d is not an object\n", i);
            cJSON_Delete(root);
            return false;
        }
        if (!validate_vector_object(item)) {
            fprintf(stderr, "Invalid vector field set at index %d\n", i);
            cJSON_Delete(root);
            return false;
        }
    }

    if (duplicate_ids_exist(vectors)) {
        fprintf(stderr, "Duplicate KAT IDs detected\n");
        cJSON_Delete(root);
        return false;
    }

    cJSON_Delete(root);
    return true;
}

static char *read_entire_file(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }
    rewind(fp);

    char *buf = (char *)malloc((size_t)size + 1U);
    if (!buf) {
        fclose(fp);
        return NULL;
    }

    size_t nread = fread(buf, 1, (size_t)size, fp);
    buf[nread] = '\0';
    fclose(fp);
    return buf;
}

static void negative_duplicate_id_test(void) {
    const char *dup_json =
        "{"
        "  \"vectors\": ["
        "    {\"id\":\"A\",\"timestamp\":\"2026-11-01T00:00:00Z\",\"berlin\":\"000000000000000000000000\",\"rgb\":[0,0,0],\"hex\":\"000000\"},"
        "    {\"id\":\"A\",\"timestamp\":\"1990-11-03T05:58:00Z\",\"berlin\":\"010000000111111111111110\",\"rgb\":[63,255,238],\"hex\":\"3FFFEE\"}"
        "  ]"
        "}";

    if (parse_and_validate_corpus(dup_json)) {
        fprintf(stderr, "Duplicate ID negative test unexpectedly passed\n");
        exit(1);
    }
}

static void negative_invalid_json_test(void) {
    const char *bad_json = "{ \"vectors\": [ { \"id\": \"A\" , \"timestamp\": \"2026-11-01T00:00:00Z\" ";
    if (parse_and_validate_corpus(bad_json)) {
        fprintf(stderr, "Invalid JSON negative test unexpectedly passed\n");
        exit(1);
    }
}

int main(void) {
    char *json_text = read_entire_file("../vectors/bu_kat_001.json");
    if (json_text == NULL) {
        fprintf(stderr, "Failed to read canonical KAT JSON\n");
        return 1;
    }

    if (!parse_and_validate_corpus(json_text)) {
        free(json_text);
        fprintf(stderr, "Canonical KAT corpus validation failed\n");
        return 1;
    }

    free(json_text);

    negative_duplicate_id_test();
    negative_invalid_json_test();

    printf("PASS: canonical KAT corpus validated and duplicate ID rejection works\n");
    return 0;
}

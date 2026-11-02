#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BIT_LEN 24

typedef struct {
    char timestamp[32];
    char berlin[32];
    int rgb[3];
    char hex[7];
} KatVector;

static size_t read_file(const char *path, char **out_ptr) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        *out_ptr = NULL;
        return 0;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        *out_ptr = NULL;
        return 0;
    }
    long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        *out_ptr = NULL;
        return 0;
    }
    rewind(fp);

    char *buf = (char *)malloc((size_t)size + 1);
    if (buf == NULL) {
        fclose(fp);
        *out_ptr = NULL;
        return 0;
    }

    size_t read_count = fread(buf, 1, (size_t)size, fp);
    buf[read_count] = '\0';
    fclose(fp);
    *out_ptr = buf;
    return read_count;
}

static void extract_json_string(const char *start, const char *key, char *out, size_t out_size) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\": \"", key);
    const char *pos = strstr(start, pattern);
    if (pos == NULL) {
        out[0] = '\0';
        return;
    }
    pos += strlen(pattern);
    size_t i = 0;
    while (*pos != '\0' && *pos != '"' && i + 1 < out_size) {
        out[i++] = *pos++;
    }
    out[i] = '\0';
}

static void extract_json_rgb(const char *start, int rgb[3]) {
    const char *pos = strstr(start, "\"rgb\": ");
    if (pos == NULL) {
        rgb[0] = rgb[1] = rgb[2] = 0;
        return;
    }
    int r = 0, g = 0, b = 0;
    if (sscanf(pos, "\"rgb\": [%d, %d, %d]", &r, &g, &b) == 3) {
        rgb[0] = r; rgb[1] = g; rgb[2] = b;
    } else {
        rgb[0] = rgb[1] = rgb[2] = 0;
    }
}

static int load_kat_vectors(KatVector *out, int max_vectors) {
    char *json = NULL;
    if (read_file("../vectors/bu_kat_001.json", &json) == 0) {
        return 0;
    }

    const char *cursor = json;
    int count = 0;
    while (count < max_vectors) {
        const char *ts_pos = strstr(cursor, "\"timestamp\": ");
        const char *berlin_pos = strstr(cursor, "\"berlin\": ");
        if (ts_pos == NULL || berlin_pos == NULL) {
            break;
        }

        KatVector kv;
        extract_json_string(ts_pos, "timestamp", kv.timestamp, sizeof(kv.timestamp));
        extract_json_string(berlin_pos, "berlin", kv.berlin, sizeof(kv.berlin));
        extract_json_rgb(ts_pos, kv.rgb);
        const char *hex_pos = strstr(berlin_pos, "\"hex\": ");
        if (hex_pos == NULL) {
            free(json);
            return count;
        }
        extract_json_string(hex_pos, "hex", kv.hex, sizeof(kv.hex));

        out[count++] = kv;
        cursor = berlin_pos + 1;
    }

    free(json);
    return count;
}

static int is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int days_in_month(int year, int month) {
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && is_leap_year(year)) {
        return 29;
    }
    return days[month - 1];
}

static int parse_timestamp(const char *ts, int *year, int *month, int *day, int *hour, int *minute, int *second) {
    if (ts == NULL || strlen(ts) != 20) {
        return 0;
    }

    if (ts[4] != '-' || ts[7] != '-' || ts[10] != 'T' ||
        ts[13] != ':' || ts[16] != ':' || ts[19] != 'Z') {
        return 0;
    }

    for (int i = 0; i < 20; ++i) {
        if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16 || i == 19) {
            continue;
        }
        if (ts[i] < '0' || ts[i] > '9') {
            return 0;
        }
    }

    char tmp[5];
    memcpy(tmp, ts, 4);
    tmp[4] = '\0';
    *year = atoi(tmp);
    if (*year < 1 || *year > 9999) {
        return 0;
    }

    memcpy(tmp, ts + 5, 2);
    tmp[2] = '\0';
    *month = atoi(tmp);

    memcpy(tmp, ts + 8, 2);
    tmp[2] = '\0';
    *day = atoi(tmp);

    memcpy(tmp, ts + 11, 2);
    tmp[2] = '\0';
    *hour = atoi(tmp);

    memcpy(tmp, ts + 14, 2);
    tmp[2] = '\0';
    *minute = atoi(tmp);

    memcpy(tmp, ts + 17, 2);
    tmp[2] = '\0';
    *second = atoi(tmp);

    if (*month < 1 || *month > 12) return 0;
    if (*day < 1 || *day > days_in_month(*year, *month)) return 0;
    if (*hour < 0 || *hour > 23) return 0;
    if (*minute < 0 || *minute > 59) return 0;
    if (*second < 0 || *second > 59) return 0;

    return 1;
}

static char *prefix_on(int count, int width) {
    char *buf = (char *)malloc((size_t)width + 1);
    if (buf == NULL) {
        return NULL;
    }
    for (int i = 0; i < width; ++i) {
        buf[i] = (i < count) ? '1' : '0';
    }
    buf[width] = '\0';
    return buf;
}

static int count_ones(const char *row, int len) {
    int count = 0;
    for (int i = 0; i < len; ++i) {
        if (row[i] == '1') {
            count++;
        }
    }
    return count;
}

static int is_prefix_on(const char *row, int len) {
    int seen_zero = 0;
    for (int i = 0; i < len; ++i) {
        if (row[i] == '1') {
            if (seen_zero) {
                return 0;
            }
            continue;
        }
        if (row[i] == '0') {
            seen_zero = 1;
            continue;
        }
        return 0;
    }
    return 1;
}

static int validate_berlin_state(const char *bits) {
    if (bits == NULL || strlen(bits) != BIT_LEN) {
        return 0;
    }

    for (size_t i = 0; i < strlen(bits); ++i) {
        if (bits[i] != '0' && bits[i] != '1') {
            return 0;
        }
    }

    const char *five_hour = bits + 1;
    const char *one_hour = bits + 5;
    const char *five_minute = bits + 9;
    const char *one_minute = bits + 20;

    if (!is_prefix_on(five_hour, 4)) return 0;
    if (!is_prefix_on(one_hour, 4)) return 0;
    if (!is_prefix_on(five_minute, 11)) return 0;
    if (!is_prefix_on(one_minute, 4)) return 0;

    int h = 5 * count_ones(five_hour, 4) + count_ones(one_hour, 4);
    int m = 5 * count_ones(five_minute, 11) + count_ones(one_minute, 4);

    if (h > 23) return 0;
    if (m > 59) return 0;

    return 1;
}

static unsigned long bits_to_ulong(const char *bits, int start, int len) {
    char *buf = (char *)malloc((size_t)len + 1);
    if (buf == NULL) {
        return 0;
    }
    memcpy(buf, bits + start, (size_t)len);
    buf[len] = '\0';
    char *end = NULL;
    unsigned long value = strtoul(buf, &end, 2);
    free(buf);
    return value;
}

static int cmap_rgb_from_bits(const char *bits, int *r, int *g, int *b) {
    if (!validate_berlin_state(bits)) {
        return 0;
    }

    unsigned long r_s = bits_to_ulong(bits, 0, 9);
    unsigned long g_s = bits_to_ulong(bits, 9, 11);
    unsigned long b_s = bits_to_ulong(bits, 20, 4);

    *r = (int)((r_s * 255) / 511);
    *g = (int)((g_s * 255) / 2047);
    *b = (int)((b_s * 255) / 15);
    return 1;
}

static void format_hex(int r, int g, int b, char out[7]) {
    snprintf(out, 7, "%02X%02X%02X", r, g, b);
}

static char *bu_encode(const char *timestamp) {
    int year, month, day, hour, minute, second;
    if (!parse_timestamp(timestamp, &year, &month, &day, &hour, &minute, &second)) {
        return NULL;
    }

    char *five_hour = prefix_on(hour / 5, 4);
    char *one_hour = prefix_on(hour % 5, 4);
    char *five_minute = prefix_on(minute / 5, 11);
    char *one_minute = prefix_on(minute % 5, 4);

    if (five_hour == NULL || one_hour == NULL || five_minute == NULL || one_minute == NULL) {
        free(five_hour);
        free(one_hour);
        free(five_minute);
        free(one_minute);
        return NULL;
    }

    char *out = (char *)malloc(BIT_LEN + 1);
    if (out == NULL) {
        free(five_hour);
        free(one_hour);
        free(five_minute);
        free(one_minute);
        return NULL;
    }

    snprintf(out, BIT_LEN + 1, "%d%s%s%s%s",
             second % 2,
             five_hour,
             one_hour,
             five_minute,
             one_minute);

    free(five_hour);
    free(one_hour);
    free(five_minute);
    free(one_minute);

    return out;
}

static int bu_validate(const char *timestamp, const char *bits) {
    char *expected = bu_encode(timestamp);
    if (expected == NULL) {
        return 0;
    }
    int ok = validate_berlin_state(bits) && strcmp(expected, bits) == 0;
    free(expected);
    return ok;
}

static void test_kat_vectors(void) {
    KatVector vectors[16];
    int count = load_kat_vectors(vectors, 16);
    if (count != 5) {
        fprintf(stderr, "Expected 5 vectors, found %d\n", count);
        exit(1);
    }

    for (int i = 0; i < count; ++i) {
        char *generated = bu_encode(vectors[i].timestamp);
        if (generated == NULL) {
            fprintf(stderr, "Generated NULL for %s\n", vectors[i].timestamp);
            exit(1);
        }
        if (strcmp(generated, vectors[i].berlin) != 0) {
            fprintf(stderr, "Mismatch for %s: got %s expected %s\n", vectors[i].timestamp, generated, vectors[i].berlin);
            free(generated);
            exit(1);
        }
        if (!bu_validate(vectors[i].timestamp, vectors[i].berlin)) {
            fprintf(stderr, "Validation failed for %s\n", vectors[i].timestamp);
            free(generated);
            exit(1);
        }
        int r = 0, g = 0, b = 0;
        if (!cmap_rgb_from_bits(vectors[i].berlin, &r, &g, &b)) {
            fprintf(stderr, "CMAP failed for %s\n", vectors[i].timestamp);
            free(generated);
            exit(1);
        }
        char hex[7];
        format_hex(r, g, b, hex);
        if (r != vectors[i].rgb[0] || g != vectors[i].rgb[1] || b != vectors[i].rgb[2]) {
            fprintf(stderr, "CMAP mismatch for %s: got %d,%d,%d expected %d,%d,%d\n",
                    vectors[i].timestamp, r, g, b,
                    vectors[i].rgb[0], vectors[i].rgb[1], vectors[i].rgb[2]);
            free(generated);
            exit(1);
        }
        if (strcmp(hex, vectors[i].hex) != 0) {
            fprintf(stderr, "HEX mismatch for %s: got %s expected %s\n",
                    vectors[i].timestamp, hex, vectors[i].hex);
            free(generated);
            exit(1);
        }
        free(generated);
    }
}

static void test_negative_cases(void) {
    if (bu_encode("2026-02-29T12:00:00Z") != NULL) exit(1);
    if (bu_encode("2026-04-31T12:00:00Z") != NULL) exit(1);
    if (bu_encode("2026-13-01T12:00:00Z") != NULL) exit(1);
    if (bu_encode("2026-00-01T12:00:00Z") != NULL) exit(1);
    if (bu_encode("0000-01-01T12:00:00Z") != NULL) exit(1);

    if (bu_encode("2026-11-01T24:00:00Z") != NULL) exit(1);
    if (bu_encode("2026-11-01T12:60:00Z") != NULL) exit(1);
    if (bu_encode("2026-11-01T12:00:60Z") != NULL) exit(1);

    if (bu_encode("2026-11-01T12:00:00.1Z") != NULL) exit(1);
    if (bu_encode("2026-11-01T12:00:00+01:00") != NULL) exit(1);
    if (bu_encode("2026-11-01T12:00:00") != NULL) exit(1);

    const char *short_bits = "00000000000000000000000";
    if (validate_berlin_state(short_bits) != 0) exit(1);

    const char *bad_char_bits = "00000000000000000000000X";
    if (validate_berlin_state(bad_char_bits) != 0) exit(1);
}

int main(void) {
    test_kat_vectors();
    test_negative_cases();
    printf("PASS: FEL-BU-1.0 KAT vectors, CMAP and negative tests satisfied.\n");
    return 0;
}

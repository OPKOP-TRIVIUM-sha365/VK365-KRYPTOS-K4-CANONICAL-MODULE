#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define KAT_COUNT 5
#define BITS_LEN 24

typedef struct {
    const char *id;
    const char *timestamp;
    const char *berlin;
    int rgb[3];
    const char *hex;
} KAT;

static bool is_digit_ascii(char c) { return c >= '0' && c <= '9'; }
static int leap(int year) { return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0); }
static int parse_num(const char *s, int start, int count) {
    int value = 0;
    for (int i = 0; i < count; ++i) {
        if (!is_digit_ascii(s[start + i])) return -1;
        value = value * 10 + s[start + i] - '0';
    }
    return value;
}

static bool parse_timestamp(const char *s, int *hour, int *minute, int *second) {
    if (s == NULL || strlen(s) != 20 || s[4] != '-' || s[7] != '-' || s[10] != 'T'
        || s[13] != ':' || s[16] != ':' || s[19] != 'Z') return false;
    for (int i = 0; i < 19; ++i) {
        if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16) continue;
        if (!is_digit_ascii(s[i])) return false;
    }
    int year = parse_num(s, 0, 4), month = parse_num(s, 5, 2), day = parse_num(s, 8, 2);
    *hour = parse_num(s, 11, 2); *minute = parse_num(s, 14, 2); *second = parse_num(s, 17, 2);
    if (year < 1 || month < 1 || month > 12 || *hour < 0 || *hour > 23
        || *minute < 0 || *minute > 59 || *second < 0 || *second > 59) return false;
    const int month_days[] = {31, leap(year) ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return day >= 1 && day <= month_days[month - 1];
}

static void append_prefix(char *out, size_t *position, int count, int width) {
    for (int i = 0; i < width; ++i) out[(*position)++] = i < count ? '1' : '0';
}

static bool encode_timestamp(const char *timestamp, char out[BITS_LEN + 1]) {
    int hour, minute, second;
    if (!parse_timestamp(timestamp, &hour, &minute, &second)) return false;
    size_t pos = 0;
    out[pos++] = (char)('0' + second % 2);
    append_prefix(out, &pos, hour / 5, 4); append_prefix(out, &pos, hour % 5, 4);
    append_prefix(out, &pos, minute / 5, 11); append_prefix(out, &pos, minute % 5, 4);
    out[pos] = '\0';
    return pos == BITS_LEN;
}

static bool channel_value(const char *bits, size_t start, size_t width, unsigned *out) {
    unsigned value = 0;
    for (size_t i = start; i < start + width; ++i) {
        if (bits[i] != '0' && bits[i] != '1') return false;
        value = value * 2U + (unsigned)(bits[i] - '0');
    }
    *out = value;
    return true;
}

static bool cmap(const char *bits, int rgb[3], char hex[7]) {
    if (bits == NULL || strlen(bits) != BITS_LEN) return false;
    unsigned r, g, b;
    if (!channel_value(bits, 0, 9, &r) || !channel_value(bits, 9, 11, &g)
        || !channel_value(bits, 20, 4, &b)) return false;
    rgb[0] = (int)(r * 255U / 511U); rgb[1] = (int)(g * 255U / 2047U); rgb[2] = (int)(b * 255U / 15U);
    (void)snprintf(hex, 7, "%02X%02X%02X", rgb[0], rgb[1], rgb[2]);
    return true;
}

static int test_kat(const char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) { perror("fixture open"); return 0; }
    char line[512]; int count = 0, failures = 0;
    while (fgets(line, sizeof line, file) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        line[strcspn(line, "\r\n")] = '\0';
        char *fields[5]; int field_count = 0; fields[field_count++] = line;
        for (char *p = line; *p != '\0'; ++p) {
            if (*p == '\t') { *p = '\0'; if (field_count < 5) fields[field_count++] = p + 1; else ++field_count; }
        }
        if (field_count != 5) { fprintf(stderr, "FAIL malformed fixture row\n"); ++failures; continue; }
        char bits[BITS_LEN + 1], hex[7]; int rgb[3];
        if (!encode_timestamp(fields[1], bits) || !cmap(bits, rgb, hex)) {
            fprintf(stderr, "FAIL vector %s computation\n", fields[0]); ++failures; continue;
        }
        int er = -1, eg = -1, eb = -1;
        if (sscanf(fields[3], "%d,%d,%d", &er, &eg, &eb) != 3 || strcmp(bits, fields[2]) != 0
            || rgb[0] != er || rgb[1] != eg || rgb[2] != eb || strcmp(hex, fields[4]) != 0) {
            fprintf(stderr, "FAIL vector %s: bits=%s rgb=%d,%d,%d hex=%s\n", fields[0], bits, rgb[0], rgb[1], rgb[2], hex);
            ++failures;
        } else { printf("PASS KAT %s\n", fields[0]); }
        ++count;
    }
    fclose(file);
    if (count != KAT_COUNT) { fprintf(stderr, "FAIL expected %d vectors, got %d\n", KAT_COUNT, count); ++failures; }
    return failures == 0;
}

static bool test_negative(void) {
    int hour, minute, second, rgb[3]; char hex[7];
    if (parse_timestamp("2026-11-01T24:00:00Z", &hour, &minute, &second)) return false;
    if (parse_timestamp("1900-02-29T00:00:00Z", &hour, &minute, &second)) return false;
    if (!parse_timestamp("2000-02-29T00:00:00Z", &hour, &minute, &second)) return false;
    if (parse_timestamp("2026-11-01T12:00:00+00:00", &hour, &minute, &second)) return false;
    if (cmap("00000000000000000000000X", rgb, hex)) return false;
    return true;
}

int main(int argc, char **argv) {
    const char *fixture = argc > 1 ? argv[1] : "fixtures/bu_kat_001.tsv";
    bool kat_ok = test_kat(fixture), negative_ok = test_negative();
    if (negative_ok) puts("PASS negative and Gregorian boundary tests");
    else fputs("FAIL negative and Gregorian boundary tests\n", stderr);
    if (kat_ok && negative_ok) { puts("C11 CONFORMANCE PASS"); return 0; }
    puts("C11 CONFORMANCE FAIL"); return 1;
}

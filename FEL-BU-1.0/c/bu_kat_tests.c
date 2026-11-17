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

/* Mirror the canonical BU-KAT-001 values. Every vector is recomputed below. */
static const KAT vectors[KAT_COUNT] = {
    {"BU-KAT-001-A", "2026-11-01T00:00:00Z", "000000000000000000000000", {0, 0, 0}, "000000"},
    {"BU-KAT-001-B", "1990-11-03T05:58:00Z", "010000000111111111111110", {63, 255, 238}, "3FFFEE"},
    {"BU-KAT-001-C", "2026-10-24T08:26:00Z", "010001110111110000001000", {70, 247, 136}, "46F788"},
    {"BU-KAT-001-D", "2026-11-01T21:00:00Z", "011111000000000000000000", {123, 0, 0}, "7B0000"},
    {"BU-KAT-001-E", "2026-11-01T23:59:59Z", "111111110111111111111111", {254, 255, 255}, "FEFFFF"}
};

static bool parse_timestamp(const char *ts, int *hour, int *minute, int *second) {
    if (ts == NULL || strlen(ts) != 20 ||
        ts[4] != '-' || ts[7] != '-' || ts[10] != 'T' ||
        ts[13] != ':' || ts[16] != ':' || ts[19] != 'Z') return false;
    for (int i = 0; i < 19; ++i) {
        if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16) continue;
        if (ts[i] < '0' || ts[i] > '9') return false;
    }
    int year = (ts[0]-'0')*1000 + (ts[1]-'0')*100 + (ts[2]-'0')*10 + ts[3]-'0';
    int month = (ts[5]-'0')*10 + ts[6]-'0';
    int day = (ts[8]-'0')*10 + ts[9]-'0';
    *hour = (ts[11]-'0')*10 + ts[12]-'0';
    *minute = (ts[14]-'0')*10 + ts[15]-'0';
    *second = (ts[17]-'0')*10 + ts[18]-'0';
    if (year < 1 || month < 1 || month > 12 || *hour > 23 || *minute > 59 || *second > 59) return false;
    bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    int month_days[] = {31, leap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return day >= 1 && day <= month_days[month - 1];
}

static void append_prefix(char *out, size_t *position, int count, int width) {
    for (int i = 0; i < width; ++i) out[(*position)++] = i < count ? '1' : '0';
}

static bool encode_timestamp(const char *ts, char out[BITS_LEN + 1]) {
    int hour, minute, second;
    if (!parse_timestamp(ts, &hour, &minute, &second)) return false;
    size_t p = 0;
    out[p++] = (char)('0' + second % 2);
    append_prefix(out, &p, hour / 5, 4);
    append_prefix(out, &p, hour % 5, 4);
    append_prefix(out, &p, minute / 5, 11);
    append_prefix(out, &p, minute % 5, 4);
    out[p] = '\0';
    return p == BITS_LEN;
}

static bool decode_channel(const char *bits, size_t start, size_t width, unsigned *out) {
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
    if (!decode_channel(bits, 0, 9, &r) || !decode_channel(bits, 9, 11, &g) || !decode_channel(bits, 20, 4, &b)) return false;
    rgb[0] = (int)((r * 255U) / 511U);
    rgb[1] = (int)((g * 255U) / 2047U);
    rgb[2] = (int)((b * 255U) / 15U);
    (void)snprintf(hex, 7, "%02X%02X%02X", rgb[0], rgb[1], rgb[2]);
    return true;
}

static int fail(const char *id, const char *what) {
    fprintf(stderr, "FAIL %s: %s\n", id, what);
    return 1;
}

int main(void) {
    for (size_t i = 0; i < KAT_COUNT; ++i) {
        char bits[BITS_LEN + 1];
        int rgb[3];
        char hex[7];
        if (!encode_timestamp(vectors[i].timestamp, bits)) return fail(vectors[i].id, "timestamp rejected");
        if (strcmp(bits, vectors[i].berlin) != 0) return fail(vectors[i].id, "Berlin bits mismatch");
        if (!cmap(bits, rgb, hex)) return fail(vectors[i].id, "CMAP rejected bits");
        for (size_t channel = 0; channel < 3; ++channel) {
            if (rgb[channel] != vectors[i].rgb[channel]) return fail(vectors[i].id, "RGB mismatch");
        }
        if (strcmp(hex, vectors[i].hex) != 0) return fail(vectors[i].id, "HEX mismatch");
        printf("PASS %s Berlin/RGB/HEX\n", vectors[i].id);
    }
    {
        int hour, minute, second, rgb[3];
        char hex[7];
        if (parse_timestamp("2026-02-29T12:00:00Z", &hour, &minute, &second)) return fail("negative", "invalid date accepted");
        if (parse_timestamp("2026-11-01T24:00:00Z", &hour, &minute, &second)) return fail("negative", "hour overflow accepted");
        if (parse_timestamp("2026-11-01T12:00:00+00:00", &hour, &minute, &second)) return fail("negative", "timezone offset accepted");
        if (cmap("00000000000000000000000X", rgb, hex)) return fail("negative", "non-binary bit accepted");
    }
    puts("PASS negative timestamp and bitstream checks");
    return 0;
}

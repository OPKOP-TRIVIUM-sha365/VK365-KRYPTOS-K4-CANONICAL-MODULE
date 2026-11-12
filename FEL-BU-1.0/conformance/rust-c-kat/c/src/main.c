/*
 * FEL-BU-1.0 — independent C11 semantic KAT implementation
 *
 * Scope:
 *   - Strictly validates the normative TSV fixture.
 *   - Independently parses UTC timestamps and computes Berlin-Uhr bits.
 *   - Applies the normative integer CMAP formulas and emits uppercase HEX.
 *   - Checks the five unchanged normative KAT vectors in order A–E.
 *
 * Build:
 *   cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
 *      -O2 -o fel_bu_c_kat c/src/main.c
 *
 * Run from the rust-c-kat directory:
 *   ./fel_bu_c_kat fixtures/bu_kat_001.tsv
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define KAT_COUNT 5
#define BITS_LEN 24
#define HEX_LEN 6
#define MAX_LINE_LEN 511

#define TSV_HEADER "id\ttimestamp\tberlin_bits\trgb\thex"

typedef struct {
    const char *id;
    const char *timestamp;
    const char *berlin_bits;
    const char *rgb;
    const char *hex;
} KAT;

/*
 * Fixed normative expectations. These are intentionally not generated from
 * the fixture: the fixture is checked against this independent expected set.
 * Do not modify these values without a normative specification change.
 */
static const KAT EXPECTED_KATS[KAT_COUNT] = {
    {"A", "2026-11-01T00:00:00Z",
     "000000000000000000000000", "0,0,0", "000000"},
    {"B", "1990-11-03T05:58:00Z",
     "010000000111111111111110", "63,255,238", "3FFFEE"},
    {"C", "2026-10-24T08:26:00Z",
     "010001110111110000001000", "70,247,136", "46F788"},
    {"D", "2026-11-01T21:00:00Z",
     "011111000000000000000000", "123,0,0", "7B0000"},
    {"E", "2026-11-01T23:59:59Z",
     "111111110111111111111111", "254,255,255", "FEFFFF"}
};

static bool is_digit_ascii(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_upper_hex_ascii(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
}

static bool is_leap_year(int year)
{
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

static int parse_fixed_decimal(const char *s, size_t start, size_t width)
{
    int value = 0;
    size_t i;

    for (i = 0; i < width; ++i) {
        char c = s[start + i];

        if (!is_digit_ascii(c)) {
            return -1;
        }

        value = value * 10 + (int)(c - '0');
    }

    return value;
}

/*
 * Strict canonical timestamp:
 * YYYY-MM-DDTHH:MM:SSZ, exactly 20 ASCII bytes.
 * Years: 0001–9999; Gregorian calendar; no offsets/fractions/leap seconds.
 */
static bool parse_timestamp(
    const char *s,
    int *hour,
    int *minute,
    int *second)
{
    int year;
    int month;
    int day;
    int parsed_hour;
    int parsed_minute;
    int parsed_second;
    int month_days[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    if (s == NULL || hour == NULL || minute == NULL || second == NULL) {
        return false;
    }

    if (strlen(s) != 20U ||
        s[4] != '-' || s[7] != '-' || s[10] != 'T' ||
        s[13] != ':' || s[16] != ':' || s[19] != 'Z') {
        return false;
    }

    year = parse_fixed_decimal(s, 0U, 4U);
    month = parse_fixed_decimal(s, 5U, 2U);
    day = parse_fixed_decimal(s, 8U, 2U);
    parsed_hour = parse_fixed_decimal(s, 11U, 2U);
    parsed_minute = parse_fixed_decimal(s, 14U, 2U);
    parsed_second = parse_fixed_decimal(s, 17U, 2U);

    if (year < 1 || month < 1 || month > 12 ||
        day < 1 || parsed_hour < 0 || parsed_hour > 23 ||
        parsed_minute < 0 || parsed_minute > 59 ||
        parsed_second < 0 || parsed_second > 59) {
        return false;
    }

    if (is_leap_year(year)) {
        month_days[1] = 29;
    }

    if (day > month_days[month - 1]) {
        return false;
    }

    *hour = parsed_hour;
    *minute = parsed_minute;
    *second = parsed_second;
    return true;
}

static void append_prefix(
    char *out,
    size_t *position,
    int count,
    int width)
{
    int i;

    for (i = 0; i < width; ++i) {
        out[(*position)++] = (i < count) ? '1' : '0';
    }
}

/* Normative 24-bit Berlin-Uhr encoding. */
static bool encode_timestamp(
    const char *timestamp,
    char out[BITS_LEN + 1])
{
    int hour;
    int minute;
    int second;
    size_t position = 0U;

    if (out == NULL ||
        !parse_timestamp(timestamp, &hour, &minute, &second)) {
        return false;
    }

    out[position++] = (char)('0' + (second % 2));
    append_prefix(out, &position, hour / 5, 4);
    append_prefix(out, &position, hour % 5, 4);
    append_prefix(out, &position, minute / 5, 11);
    append_prefix(out, &position, minute % 5, 4);

    out[position] = '\0';
    return position == BITS_LEN;
}

static bool channel_value(
    const char *bits,
    size_t start,
    size_t width,
    unsigned *out)
{
    unsigned value = 0U;
    size_t i;

    if (bits == NULL || out == NULL) {
        return false;
    }

    for (i = start; i < start + width; ++i) {
        if (bits[i] != '0' && bits[i] != '1') {
            return false;
        }

        value = value * 2U + (unsigned)(bits[i] - '0');
    }

    *out = value;
    return true;
}

/*
 * Normative CMAP:
 *   R = (Rs * 255) / 511
 *   G = (Gs * 255) / 2047
 *   B = (Bs * 255) / 15
 * Integer division is intentional.
 */
static bool cmap(
    const char *bits,
    int rgb[3],
    char hex[HEX_LEN + 1])
{
    unsigned r_source;
    unsigned g_source;
    unsigned b_source;

    if (bits == NULL || rgb == NULL || hex == NULL ||
        strlen(bits) != BITS_LEN) {
        return false;
    }

    if (!channel_value(bits, 0U, 9U, &r_source) ||
        !channel_value(bits, 9U, 11U, &g_source) ||
        !channel_value(bits, 20U, 4U, &b_source)) {
        return false;
    }

    rgb[0] = (int)((r_source * 255U) / 511U);
    rgb[1] = (int)((g_source * 255U) / 2047U);
    rgb[2] = (int)((b_source * 255U) / 15U);

    {
        static const char digits[] = "0123456789ABCDEF";
        unsigned channels[3];

        channels[0] = (unsigned)rgb[0];
        channels[1] = (unsigned)rgb[1];
        channels[2] = (unsigned)rgb[2];

        hex[0] = digits[(channels[0] >> 4U) & 0x0FU];
        hex[1] = digits[channels[0] & 0x0FU];
        hex[2] = digits[(channels[1] >> 4U) & 0x0FU];
        hex[3] = digits[channels[1] & 0x0FU];
        hex[4] = digits[(channels[2] >> 4U) & 0x0FU];
        hex[5] = digits[channels[2] & 0x0FU];
        hex[6] = '\0';
    }

    return strlen(hex) == HEX_LEN;
}

/*
 * Strict canonical RGB parser:
 *   - exactly three decimal channels separated by commas;
 *   - each channel 0–255;
 *   - no signs, spaces, empty channels, extra characters, or leading zeroes
 *     except the single-character representation "0".
 */
static bool parse_rgb(const char *s, int rgb[3])
{
    size_t pos = 0U;
    int channel_index;

    if (s == NULL || rgb == NULL) {
        return false;
    }

    for (channel_index = 0; channel_index < 3; ++channel_index) {
        int value = 0;
        size_t digits = 0U;

        if (!is_digit_ascii(s[pos])) {
            return false;
        }

        if (s[pos] == '0' && is_digit_ascii(s[pos + 1U])) {
            return false;
        }

        while (is_digit_ascii(s[pos])) {
            value = value * 10 + (int)(s[pos] - '0');
            if (value > 255) {
                return false;
            }
            ++pos;
            ++digits;
        }

        if (digits == 0U) {
            return false;
        }

        rgb[channel_index] = value;

        if (channel_index < 2) {
            if (s[pos] != ',') {
                return false;
            }
            ++pos;
        }
    }

    return s[pos] == '\0';
}

static bool is_valid_hex(const char *s)
{
    size_t i;

    if (s == NULL || strlen(s) != HEX_LEN) {
        return false;
    }

    for (i = 0U; i < HEX_LEN; ++i) {
        if (!is_upper_hex_ascii(s[i])) {
            return false;
        }
    }

    return true;
}

/* Split exactly five TAB-separated fields, rejecting missing/extra fields. */
static bool split_tsv_row(char *row, char *fields[5])
{
    size_t count = 1U;
    char *p;

    if (row == NULL || fields == NULL) {
        return false;
    }

    fields[0] = row;

    for (p = row; *p != '\0'; ++p) {
        if (*p == '\t') {
            if (count >= 5U) {
                return false;
            }

            *p = '\0';
            fields[count++] = p + 1;
        }
    }

    if (count != 5U) {
        return false;
    }

    for (count = 0U; count < 5U; ++count) {
        if (fields[count][0] == '\0') {
            return false;
        }
    }

    return true;
}

static bool validate_row(
    const char *row,
    const KAT *expected)
{
    char copy[MAX_LINE_LEN + 1];
    char *fields[5];
    char computed_bits[BITS_LEN + 1];
    char computed_hex[HEX_LEN + 1];
    int computed_rgb[3];
    int fixture_rgb[3];

    if (row == NULL || expected == NULL ||
        strlen(row) > MAX_LINE_LEN) {
        return false;
    }

    (void)snprintf(copy, sizeof copy, "%s", row);

    if (!split_tsv_row(copy, fields)) {
        return false;
    }

    /* Check the fixed normative record, not an ID copied from the row. */
    if (strcmp(fields[0], expected->id) != 0 ||
        strcmp(fields[1], expected->timestamp) != 0 ||
        strcmp(fields[2], expected->berlin_bits) != 0 ||
        strcmp(fields[3], expected->rgb) != 0 ||
        strcmp(fields[4], expected->hex) != 0) {
        return false;
    }

    if (!encode_timestamp(fields[1], computed_bits) ||
        !cmap(computed_bits, computed_rgb, computed_hex) ||
        !parse_rgb(fields[3], fixture_rgb) ||
        !is_valid_hex(fields[4])) {
        return false;
    }

    return strcmp(computed_bits, fields[2]) == 0 &&
        computed_rgb[0] == fixture_rgb[0] &&
        computed_rgb[1] == fixture_rgb[1] &&
        computed_rgb[2] == fixture_rgb[2] &&
        strcmp(computed_hex, fields[4]) == 0;
}

static bool read_fixture_line(
    FILE *file,
    char line[MAX_LINE_LEN + 1],
    bool *eof)
{
    size_t length;
    int ch;

    if (fgets(line, MAX_LINE_LEN + 1, file) == NULL) {
        *eof = true;
        return false;
    }

    *eof = false;
    length = strlen(line);

    /*
     * If no newline was read and the buffer is full, determine whether the
     * physical line continues. A final line without LF is accepted only if
     * it fits completely in the buffer.
     */
    if (length == MAX_LINE_LEN &&
        line[length - 1U] != '\n') {
        ch = fgetc(file);
        if (ch != EOF) {
            while (ch != '\n' && ch != EOF) {
                ch = fgetc(file);
            }
            return false;
        }
    }

    if (length > 0U && line[length - 1U] == '\n') {
        line[--length] = '\0';
    }

    if (length > 0U && line[length - 1U] == '\r') {
        line[--length] = '\0';
    }

    return true;
}

static bool test_kat(const char *path)
{
    FILE *file;
    char line[MAX_LINE_LEN + 1];
    bool eof = false;
    size_t index;

    file = fopen(path, "r");
    if (file == NULL) {
        perror("fixture open");
        return false;
    }

    if (!read_fixture_line(file, line, &eof) ||
        strcmp(line, TSV_HEADER) != 0) {
        fprintf(stderr, "FAIL exact TSV header mismatch\n");
        (void)fclose(file);
        return false;
    }

    for (index = 0U; index < KAT_COUNT; ++index) {
        if (!read_fixture_line(file, line, &eof)) {
            fprintf(stderr, "FAIL missing, oversized, or unreadable KAT row %zu\n",
                    index + 1U);
            (void)fclose(file);
            return false;
        }

        if (!validate_row(line, &EXPECTED_KATS[index])) {
            fprintf(stderr, "FAIL KAT row %zu (expected ID %s)\n",
                    index + 1U, EXPECTED_KATS[index].id);
            (void)fclose(file);
            return false;
        }

        printf("PASS KAT %s\n", EXPECTED_KATS[index].id);
    }

    if (read_fixture_line(file, line, &eof)) {
        fprintf(stderr, "FAIL unexpected extra physical row or content\n");
        (void)fclose(file);
        return false;
    }

    if (!eof) {
        fprintf(stderr, "FAIL unreadable trailing fixture content\n");
        (void)fclose(file);
        return false;
    }

    if (ferror(file)) {
        fprintf(stderr, "FAIL fixture read error\n");
        (void)fclose(file);
        return false;
    }

    if (fclose(file) != 0) {
        perror("fixture close");
        return false;
    }

    return true;
}

static bool test_negative(void)
{
    int hour;
    int minute;
    int second;
    int rgb[3];
    char bits[BITS_LEN + 1];
    char hex[HEX_LEN + 1];

    /* Timestamp shape, ASCII, Gregorian calendar, and ranges. */
    if (parse_timestamp("2026-11-01T24:00:00Z", &hour, &minute, &second)) return false;
    if (parse_timestamp("2026-11-01T12:60:00Z", &hour, &minute, &second)) return false;
    if (parse_timestamp("2026-11-01T12:00:60Z", &hour, &minute, &second)) return false;
    if (parse_timestamp("1900-02-29T00:00:00Z", &hour, &minute, &second)) return false;
    if (parse_timestamp("2026-02-29T00:00:00Z", &hour, &minute, &second)) return false;
    if (!parse_timestamp("2000-02-29T00:00:00Z", &hour, &minute, &second)) return false;
    if (!parse_timestamp("2024-02-29T00:00:00Z", &hour, &minute, &second)) return false;
    if (parse_timestamp("0000-01-01T00:00:00Z", &hour, &minute, &second)) return false;
    if (parse_timestamp("2026-11-01T12:00:00+00:00", &hour, &minute, &second)) return false;
    if (parse_timestamp("2026-11-01T12:00:00.1Z", &hour, &minute, &second)) return false;
    if (parse_timestamp("2026-1１-01T12:00:00Z", &hour, &minute, &second)) return false;
    if (parse_timestamp("2026-11-01T12:00Z", &hour, &minute, &second)) return false;
    if (parse_timestamp(NULL, &hour, &minute, &second)) return false;
    if (parse_timestamp("2026-11-01T12:00:00Z", NULL, &minute, &second)) return false;

    /* Positive timestamp boundary sanity checks. */
    if (!parse_timestamp("0001-01-01T00:00:00Z", &hour, &minute, &second)) return false;
    if (!parse_timestamp("9999-12-31T23:59:59Z", &hour, &minute, &second)) return false;

    /* Berlin bitstream shape and invalid input. */
    if (!encode_timestamp("1990-11-03T05:58:00Z", bits)) return false;
    if (strcmp(bits, "010000000111111111111110") != 0) return false;
    if (encode_timestamp("not-a-timestamp", bits)) return false;

    /* CMAP invalid length and non-binary input. */
    if (cmap("00000000000000000000000X", rgb, hex)) return false;
    if (cmap("00000000000000000000000", rgb, hex)) return false;
    if (cmap("0000000000000000000000000", rgb, hex)) return false;

    /* Strict RGB syntax and bounds. */
    if (!parse_rgb("0,0,0", rgb)) return false;
    if (!parse_rgb("255,255,255", rgb)) return false;
    if (parse_rgb("0,0", rgb)) return false;
    if (parse_rgb("0,0,0,0", rgb)) return false;
    if (parse_rgb("256,0,0", rgb)) return false;
    if (parse_rgb("-1,0,0", rgb)) return false;
    if (parse_rgb("+1,0,0", rgb)) return false;
    if (parse_rgb(" 1,0,0", rgb)) return false;
    if (parse_rgb("1, 0,0", rgb)) return false;
    if (parse_rgb("01,0,0", rgb)) return false;
    if (parse_rgb("1,,0", rgb)) return false;
    if (parse_rgb("1,0,0x", rgb)) return false;

    /* Strict uppercase HEX. */
    if (!is_valid_hex("000000")) return false;
    if (!is_valid_hex("FEFFFF")) return false;
    if (is_valid_hex("#000000")) return false;
    if (is_valid_hex("ffffff")) return false;
    if (is_valid_hex("00000")) return false;
    if (is_valid_hex("0000000")) return false;
    if (is_valid_hex("00000G")) return false;

    /* TSV field count, empty field, ID, and expected-value mismatch. */
    if (!validate_row(
            "A\t2026-11-01T00:00:00Z\t000000000000000000000000\t0,0,0\t000000",
            &EXPECTED_KATS[0])) return false;
    if (validate_row(
            "A\t2026-11-01T00:00:00Z\t000000000000000000000000\t0,0\t000000",
            &EXPECTED_KATS[0])) return false;
    if (validate_row(
            "A\t2026-11-01T00:00:00Z\t000000000000000000000000\t0,0,0\t000000\textra",
            &EXPECTED_KATS[0])) return false;
    if (validate_row(
            "B\t2026-11-01T00:00:00Z\t000000000000000000000000\t0,0,0\t000000",
            &EXPECTED_KATS[0])) return false;
    if (validate_row(
            "A\t2026-11-01T00:00:00Z\t000000000000000000000000\t0,0,0\tFFFFFF",
            &EXPECTED_KATS[0])) return false;
    if (validate_row(
            "A\t2026-11-01T00:00:00Z\t000000000000000000000000\t\t000000",
            &EXPECTED_KATS[0])) return false;

    return true;
}

int main(int argc, char **argv)
{
    const char *fixture =
        (argc > 1) ? argv[1] : "fixtures/bu_kat_001.tsv";
    bool kat_ok;
    bool negative_ok;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [fixture.tsv]\n", argv[0]);
        return 2;
    }

    kat_ok = test_kat(fixture);
    negative_ok = test_negative();

    if (negative_ok) {
        puts("PASS negative timestamp, Gregorian, bitstream, RGB, HEX, and TSV tests");
    } else {
        fputs("FAIL negative timestamp, Gregorian, bitstream, RGB, HEX, and TSV tests\n",
              stderr);
    }

    if (kat_ok && negative_ok) {
        puts("C11 CONFORMANCE PASS");
        return 0;
    }

    puts("C11 CONFORMANCE FAIL");
    return 1;
}

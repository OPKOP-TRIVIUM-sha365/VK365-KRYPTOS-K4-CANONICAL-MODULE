use chrono::{NaiveDateTime, Timelike};
use regex::Regex;
use serde_json::Value;
use std::fs;
use std::path::Path;

#[derive(Debug, Clone)]
struct BerlinUhrError {
    code: String,
}

impl BerlinUhrError {
    fn new(code: &str) -> Self {
        Self { code: code.to_string() }
    }
}

fn parse_timestamp(ts: &str) -> Result<NaiveDateTime, BerlinUhrError> {
    if !ts.is_ascii() {
        return Err(BerlinUhrError::new("TIMESTAMP_NON_ASCII"));
    }
    if ts.len() != 20 {
        return Err(BerlinUhrError::new("TIMESTAMP_LENGTH"));
    }

    let re = Regex::new(r"^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z$").unwrap();
    if !re.is_match(ts) {
        return Err(BerlinUhrError::new("TIMESTAMP_FORMAT"));
    }

    let year: i32 = ts[0..4].parse().unwrap_or(0);
    if !(1..=9999).contains(&year) {
        return Err(BerlinUhrError::new("TIMESTAMP_YEAR_RANGE"));
    }

    match NaiveDateTime::parse_from_str(ts, "%Y-%m-%dT%H:%M:%SZ") {
        Ok(dt) => Ok(dt),
        Err(_) => Err(BerlinUhrError::new("TIMESTAMP_VALUE")),
    }
}

fn prefix_on(count: i32, width: i32) -> Result<String, BerlinUhrError> {
    if !(0 <= count && count <= width) {
        return Err(BerlinUhrError::new("COUNT_RANGE"));
    }

    let mut out = String::with_capacity(width as usize);
    for _ in 0..count {
        out.push('1');
    }
    for _ in count..width {
        out.push('0');
    }
    Ok(out)
}

fn bu_encode(timestamp: &str) -> Result<String, BerlinUhrError> {
    let dt = parse_timestamp(timestamp)?;
    let h = dt.hour() as i32;
    let m = dt.minute() as i32;
    let s = dt.second() as i32;

    let second_bit = (s % 2).to_string();
    let five_hour = prefix_on(h / 5, 4)?;
    let one_hour = prefix_on(h % 5, 4)?;
    let five_minute = prefix_on(m / 5, 11)?;
    let one_minute = prefix_on(m % 5, 4)?;

    let result = second_bit + &five_hour + &one_hour + &five_minute + &one_minute;
    if result.len() != 24 {
        return Err(BerlinUhrError::new("INTERNAL_LENGTH"));
    }
    Ok(result)
}

fn is_prefix_on(row: &str) -> bool {
    let mut seen_zero = false;
    for ch in row.chars() {
        if ch == '1' {
            if seen_zero {
                return false;
            }
            continue;
        } else if ch == '0' {
            seen_zero = true;
            continue;
        } else {
            return false;
        }
    }
    true
}

fn validate_berlin_state(bits: &str) -> Result<(i32, i32), BerlinUhrError> {
    if bits.len() != 24 {
        return Err(BerlinUhrError::new("BERLIN_LENGTH"));
    }
    if bits.chars().any(|c| c != '0' && c != '1') {
        return Err(BerlinUhrError::new("BERLIN_BITS"));
    }

    let five_hour = &bits[1..5];
    let one_hour = &bits[5..9];
    let five_minute = &bits[9..20];
    let one_minute = &bits[20..24];

    if !is_prefix_on(five_hour) {
        return Err(BerlinUhrError::new("NON_PREFIX_ROW:FIVE_HOUR"));
    }
    if !is_prefix_on(one_hour) {
        return Err(BerlinUhrError::new("NON_PREFIX_ROW:ONE_HOUR"));
    }
    if !is_prefix_on(five_minute) {
        return Err(BerlinUhrError::new("NON_PREFIX_ROW:FIVE_MINUTE"));
    }
    if !is_prefix_on(one_minute) {
        return Err(BerlinUhrError::new("NON_PREFIX_ROW:ONE_MINUTE"));
    }

    let hour = 5 * five_hour.matches('1').count() as i32 + one_hour.matches('1').count() as i32;
    let minute = 5 * five_minute.matches('1').count() as i32 + one_minute.matches('1').count() as i32;

    if hour > 23 {
        return Err(BerlinUhrError::new("HOUR_OUT_OF_RANGE"));
    }
    if minute > 59 {
        return Err(BerlinUhrError::new("MINUTE_OUT_OF_RANGE"));
    }

    Ok((hour, minute))
}

fn bu_validate(timestamp: &str, bits: &str) -> Result<bool, BerlinUhrError> {
    let expected = bu_encode(timestamp)?;
    validate_berlin_state(bits)?;
    if bits != expected {
        return Err(BerlinUhrError::new("TIMESTAMP_BERLIN_MISMATCH"));
    }
    Ok(true)
}

fn bits_to_u32(bits: &str, start: usize, len: usize) -> u32 {
    let slice = &bits[start..start + len];
    u32::from_str_radix(slice, 2).unwrap()
}

fn cmap_rgb_from_bits(bits: &str) -> Result<(u32, u32, u32), BerlinUhrError> {
    validate_berlin_state(bits)?;
    let r_s = bits_to_u32(bits, 0, 9);
    let g_s = bits_to_u32(bits, 9, 11);
    let b_s = bits_to_u32(bits, 20, 4);

    let r = (r_s * 255) / 511;
    let g = (g_s * 255) / 2047;
    let b = (b_s * 255) / 15;
    Ok((r, g, b))
}

fn cmap_hex_from_bits(bits: &str) -> Result<String, BerlinUhrError> {
    let (r, g, b) = cmap_rgb_from_bits(bits)?;
    Ok(format!("{:02X}{:02X}{:02X}", r, g, b))
}

fn load_kat_vectors() -> Result<Vec<(String, String, Vec<u32>, String)>, String> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../vectors/bu_kat_001.json");
    let content = fs::read_to_string(path).map_err(|e| e.to_string())?;
    let json: Value = serde_json::from_str(&content).map_err(|e| e.to_string())?;

    let mut out = Vec::new();
    for item in json["vectors"].as_array().unwrap() {
        let ts = item["timestamp"].as_str().unwrap().to_string();
        let berlin = item["berlin"].as_str().unwrap().to_string();
        let rgb = item["rgb"]
            .as_array()
            .unwrap()
            .iter()
            .map(|v| v.as_u64().unwrap() as u32)
            .collect();
        let hex = item["hex"].as_str().unwrap().to_string();
        out.push((ts, berlin, rgb, hex));
    }
    Ok(out)
}

#[test]
fn kat_vectors() {
    let cases = load_kat_vectors().unwrap();
    for (ts, expected, _, _) in cases {
        assert_eq!(bu_encode(&ts).unwrap(), expected);
        assert!(bu_validate(&ts, &expected).unwrap());
    }
}

#[test]
fn cmap_kat_vectors() {
    let cases = load_kat_vectors().unwrap();
    for (ts, bits, rgb, hex) in cases {
        let (r, g, b) = cmap_rgb_from_bits(&bits).unwrap();
        let hex_value = cmap_hex_from_bits(&bits).unwrap();
        assert_eq!((r, g, b), (rgb[0], rgb[1], rgb[2]));
        assert_eq!(hex_value, hex);
        let _ = ts;
    }
}

#[test]
fn invalid_calendar_date() {
    let bad = [
        "2026-02-29T12:00:00Z",
        "2026-04-31T12:00:00Z",
        "2026-13-01T12:00:00Z",
        "2026-00-01T12:00:00Z",
        "0000-01-01T12:00:00Z",
    ];
    for ts in bad {
        assert!(bu_encode(ts).is_err());
    }
}

#[test]
fn leap_year_date_is_valid() {
    assert_eq!(bu_encode("2024-02-29T12:00:00Z").unwrap().len(), 24);
}

#[test]
fn invalid_time() {
    let bad = [
        "2026-11-01T24:00:00Z",
        "2026-11-01T12:60:00Z",
        "2026-11-01T12:00:60Z",
    ];
    for ts in bad {
        assert!(bu_encode(ts).is_err());
    }
}

#[test]
fn fractional_seconds_rejected() {
    assert!(bu_encode("2026-11-01T12:00:00.1Z").is_err());
}

#[test]
fn timezone_offset_rejected() {
    assert!(bu_encode("2026-11-01T12:00:00+01:00").is_err());
}

#[test]
fn timezone_missing_z_rejected() {
    assert!(bu_encode("2026-11-01T12:00:00").is_err());
}

#[test]
fn non_ascii_rejected() {
    assert!(bu_encode("２０２６-11-01T12:00:00Z").is_err());
}

#[test]
fn invalid_berlin_length() {
    assert!(validate_berlin_state(&"0".repeat(23)).is_err());
}

#[test]
fn invalid_berlin_character() {
    let bits = "0".repeat(23) + "X";
    assert!(validate_berlin_state(&bits).is_err());
}

#[test]
fn non_prefix_five_hour() {
    let bits = "0".to_string() + "1010" + "0000" + &"0".repeat(11) + "0000";
    assert!(validate_berlin_state(&bits).is_err());
}

#[test]
fn non_prefix_one_hour() {
    let bits = "0".to_string() + "0000" + "1010" + &"0".repeat(11) + "0000";
    assert!(validate_berlin_state(&bits).is_err());
}

#[test]
fn non_prefix_five_minute() {
    let bits = "0".to_string() + "0000" + "0000" + "10100000000" + "0000";
    assert!(validate_berlin_state(&bits).is_err());
}

#[test]
fn non_prefix_one_minute() {
    let bits = "0".to_string() + "0000" + "0000" + &"0".repeat(11) + "1010";
    assert!(validate_berlin_state(&bits).is_err());
}

#[test]
fn hour_24_rejected() {
    let bits = "0".to_string() + "1111" + "1111" + &"0".repeat(11) + "0000";
    assert!(validate_berlin_state(&bits).is_err());
}

#[test]
fn timestamp_state_mismatch() {
    let wrong = "000000000000000000000000";
    assert!(bu_validate("2026-11-01T21:00:00Z", wrong).is_err());
}

#[test]
fn second_parity() {
    let even = bu_encode("2026-11-01T12:00:00Z").unwrap();
    let odd = bu_encode("2026-11-01T12:00:01Z").unwrap();
    assert_eq!(&even[0..1], "0");
    assert_eq!(&odd[0..1], "1");
    assert_eq!(&even[1..], &odd[1..]);
}

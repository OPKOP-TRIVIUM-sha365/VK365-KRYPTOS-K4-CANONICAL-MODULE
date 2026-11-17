use serde_json::Value;
use std::collections::HashSet;
use std::fs;
use std::path::Path;

fn load_kat_document() -> Result<Value, String> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../vectors/bu_kat_001.json");
    let text = fs::read_to_string(path).map_err(|e| e.to_string())?;
    let root: Value = serde_json::from_str(&text).map_err(|e| e.to_string())?;
    let vectors = root.get("vectors").and_then(Value::as_array).ok_or("Missing vectors array")?;
    if vectors.len() != 5 {
        return Err("Expected 5 canonical KAT vectors".to_string());
    }
    let mut ids = HashSet::new();
    for vector in vectors {
        let object = vector.as_object().ok_or("Vector is not an object")?;
        let id = object.get("id").and_then(Value::as_str).ok_or("id not a string")?;
        if !ids.insert(id.to_string()) { return Err(format!("Duplicate KAT ID: {id}")); }
        let timestamp = object.get("timestamp").and_then(Value::as_str).ok_or("timestamp not a string")?;
        let bits = object.get("berlin").and_then(Value::as_str).ok_or("berlin not a string")?;
        let rgb = object.get("rgb").and_then(Value::as_array).ok_or("rgb not an array")?;
        let hex = object.get("hex").and_then(Value::as_str).ok_or("hex not a string")?;
        if timestamp.len() != 20 || bits.len() != 24 || rgb.len() != 3 || hex.len() != 6 {
            return Err(format!("Invalid vector dimensions for {id}"));
        }
        if !bits.bytes().all(|b| b == b'0' || b == b'1') { return Err(format!("Invalid bits for {id}")); }
        for channel in rgb {
            let value = channel.as_i64().ok_or("RGB channel is not an integer")?;
            if !(0..=255).contains(&value) { return Err(format!("RGB channel out of range for {id}")); }
        }
    }
    Ok(root)
}

fn digits(s: &str, start: usize, len: usize) -> Option<u32> {
    s.get(start..start + len)?.parse().ok()
}

fn encode_timestamp(ts: &str) -> Option<String> {
    let b = ts.as_bytes();
    if b.len() != 20 || !ts.is_ascii() || b[4] != b'-' || b[7] != b'-' || b[10] != b'T'
        || b[13] != b':' || b[16] != b':' || b[19] != b'Z' { return None; }
    for (i, byte) in b.iter().enumerate() {
        if ![4, 7, 10, 13, 16, 19].contains(&i) && !byte.is_ascii_digit() { return None; }
    }
    let year = digits(ts, 0, 4)?;
    let month = digits(ts, 5, 2)?;
    let day = digits(ts, 8, 2)?;
    let hour = digits(ts, 11, 2)?;
    let minute = digits(ts, 14, 2)?;
    let second = digits(ts, 17, 2)?;
    if year == 0 || !(1..=12).contains(&month) || hour > 23 || minute > 59 || second > 59 { return None; }
    let leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    let days = match month { 2 if leap => 29, 2 => 28, 4 | 6 | 9 | 11 => 30, _ => 31 };
    if day == 0 || day > days { return None; }
    Some(format!("{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}",
        second % 2, "1".repeat((hour / 5) as usize), "0".repeat(4 - (hour / 5) as usize),
        "1".repeat((hour % 5) as usize), "0".repeat(4 - (hour % 5) as usize),
        "1".repeat((minute / 5) as usize), "0".repeat(11 - (minute / 5) as usize),
        "1".repeat((minute % 5) as usize), "0".repeat(4 - (minute % 5) as usize),
        "", "", "", "", "", "", "", "", "", "", ""))
}

fn cmap(bits: &str) -> Option<([u8; 3], String)> {
    if bits.len() != 24 || !bits.bytes().all(|b| b == b'0' || b == b'1') { return None; }
    let value = |slice: &str| u32::from_str_radix(slice, 2).ok();
    let r = value(&bits[0..9])?;
    let g = value(&bits[9..20])?;
    let b = value(&bits[20..24])?;
    let rgb = [((r * 255) / 511) as u8, ((g * 255) / 2047) as u8, ((b * 255) / 15) as u8];
    let hex = format!("{:02X}{:02X}{:02X}", rgb[0], rgb[1], rgb[2]);
    Some((rgb, hex))
}

#[test]
fn five_shared_vectors_match_timestamp_berlin_cmap_and_hex() {
    let doc = load_kat_document().unwrap();
    for vector in doc["vectors"].as_array().unwrap() {
        let id = vector["id"].as_str().unwrap();
        let timestamp = vector["timestamp"].as_str().unwrap();
        let expected_bits = vector["berlin"].as_str().unwrap();
        let actual_bits = encode_timestamp(timestamp).unwrap_or_else(|| panic!("invalid timestamp in {id}"));
        assert_eq!(actual_bits, expected_bits, "Berlin KAT {id}");
        let (rgb, hex) = cmap(&actual_bits).unwrap();
        let expected_rgb: Vec<u8> = vector["rgb"].as_array().unwrap().iter().map(|x| x.as_u64().unwrap() as u8).collect();
        assert_eq!(rgb.as_slice(), expected_rgb.as_slice(), "CMAP RGB {id}");
        assert_eq!(hex, vector["hex"].as_str().unwrap(), "CMAP HEX {id}");
    }
}

#[test]
fn timestamp_validation_rejects_bad_calendar_time_and_format() {
    for timestamp in ["2026-02-29T12:00:00Z", "2026-04-31T12:00:00Z", "2026-11-01T24:00:00Z",
        "2026-11-01T12:60:00Z", "2026-11-01T12:00:60Z", "2026-11-01T12:00:00+00:00", "2024-02-29T12:00:00Z"] {
        let accepted = encode_timestamp(timestamp).is_some();
        assert_eq!(accepted, timestamp == "2024-02-29T12:00:00Z", "timestamp {timestamp}");
    }
}

#[test]
fn cmap_rejects_wrong_length_and_non_binary_input() {
    assert!(cmap("0".repeat(23).as_str()).is_none());
    assert!(cmap("00000000000000000000000X").is_none());
}

#[test]
fn cmap_full_domain_endpoints_are_exact() {
    assert_eq!(cmap(&"0".repeat(24)).unwrap(), ([0, 0, 0], "000000".to_string()));
    assert_eq!(cmap(&"1".repeat(24)).unwrap(), ([255, 255, 255], "FFFFFF".to_string()));
}

#[test]
fn canonical_vector_ids_are_unique_and_count_is_five() {
    let doc = load_kat_document().unwrap();
    assert_eq!(doc["vectors"].as_array().unwrap().len(), 5);
}

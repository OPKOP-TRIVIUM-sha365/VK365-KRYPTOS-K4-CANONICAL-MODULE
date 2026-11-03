use std::fs;

#[derive(Debug)]
struct Vector {
    id: String,
    timestamp: String,
    bits: String,
    rgb: [u8; 3],
    hex: String,
}

fn parse_timestamp(s: &str) -> Result<(u32, u32, u32, u32, u32, u32), String> {
    let b = s.as_bytes();
    if b.len() != 20 || b[4] != b'-' || b[7] != b'-' || b[10] != b'T'
        || b[13] != b':' || b[16] != b':' || b[19] != b'Z' {
        return Err("timestamp must be exact YYYY-MM-DDTHH:MM:SSZ ASCII".into());
    }
    for (i, c) in b.iter().enumerate() {
        if ![4, 7, 10, 13, 16, 19].contains(&i) && !c.is_ascii_digit() {
            return Err("non-digit timestamp field".into());
        }
    }
    let n = |a: usize, z: usize| -> u32 { s[a..z].parse().unwrap() };
    let (y, m, d, h, mi, se) = (n(0, 4), n(5, 7), n(8, 10), n(11, 13), n(14, 16), n(17, 19));
    if y == 0 || h > 23 || mi > 59 || se > 59 || m == 0 || m > 12 {
        return Err("timestamp field out of range".into());
    }
    let leap = y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
    let mdays = [31, if leap { 29 } else { 28 }, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
    if d == 0 || d > mdays[(m - 1) as usize] {
        return Err("invalid Gregorian date".into());
    }
    Ok((y, m, d, h, mi, se))
}

fn prefix(n: u32, width: usize) -> String {
    (0..width).map(|i| if i < n as usize { '1' } else { '0' }).collect()
}

fn bu(timestamp: &str) -> Result<String, String> {
    let (_, _, _, h, m, s) = parse_timestamp(timestamp)?;
    Ok(format!("{}{}{}{}{}", if s % 2 == 1 { "1" } else { "0" },
        prefix(h / 5, 4), prefix(h % 5, 4), prefix(m / 5, 11), prefix(m % 5, 4)))
}

fn cmap(bits: &str) -> Result<([u8; 3], String), String> {
    if bits.len() != 24 || !bits.bytes().all(|c| c == b'0' || c == b'1') {
        return Err("bits must be 24 binary chars".into());
    }
    let r = u16::from_str_radix(&bits[0..9], 2).unwrap();
    let g = u16::from_str_radix(&bits[9..20], 2).unwrap();
    let b = u16::from_str_radix(&bits[20..24], 2).unwrap();
    let rgb = [((r * 255) / 511) as u8, ((g * 255) / 2047) as u8, ((b * 255) / 15) as u8];
    let hex = format!("{:02X}{:02X}{:02X}", rgb[0], rgb[1], rgb[2]);
    Ok((rgb, hex))
}

fn read_vectors(path: &str) -> Vec<Vector> {
    let data = fs::read_to_string(path).expect("cannot read shared KAT fixture");
    data.lines().filter(|line| !line.is_empty() && !line.starts_with('#')).map(|line| {
        let fields: Vec<&str> = line.split('\t').collect();
        assert_eq!(fields.len(), 5, "malformed TSV row: {line}");
        let channels: Vec<u8> = fields[3].split(',').map(|x| x.parse().unwrap()).collect();
        assert_eq!(channels.len(), 3);
        Vector { id: fields[0].into(), timestamp: fields[1].into(), bits: fields[2].into(),
            rgb: [channels[0], channels[1], channels[2]], hex: fields[4].into() }
    }).collect()
}

#[test]
fn kat_001_all_five_shared_vectors() {
    let vectors = read_vectors("fixtures/bu_kat_001.tsv");
    assert_eq!(vectors.len(), 5);
    for vector in vectors {
        let bits = bu(&vector.timestamp).unwrap();
        assert_eq!(bits, vector.bits, "Berlin mismatch for {}", vector.id);
        let (rgb, hex) = cmap(&bits).unwrap();
        assert_eq!(rgb, vector.rgb, "RGB mismatch for {}", vector.id);
        assert_eq!(hex, vector.hex, "HEX mismatch for {}", vector.id);
    }
}

#[test]
fn rejects_invalid_timestamp_forms_and_accepts_leap_day() {
    for timestamp in ["2026-2-01T00:00:00Z", "2026-11-01T24:00:00Z", "2025-02-29T00:00:00Z",
        "2026-11-01T00:60:00Z", "2026-11-01T00:00:60Z", "2026-11-01T00:00:00+00:00", "0000-01-01T00:00:00Z"] {
        assert!(parse_timestamp(timestamp).is_err(), "accepted {timestamp}");
    }
    assert!(parse_timestamp("2000-02-29T12:34:56Z").is_ok());
    assert!(parse_timestamp("1900-02-29T12:34:56Z").is_err());
}

#[test]
fn rejects_invalid_bitstrings() {
    assert!(cmap("000").is_err());
    assert!(cmap("00000000000000000000000x").is_err());
}

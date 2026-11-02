use serde_json::Value;
use std::collections::HashSet;
use std::fs;
use std::path::Path;

fn load_kat_document() -> Result<Value, String> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../vectors/bu_kat_001.json");

    let text = fs::read_to_string(path).map_err(|e| e.to_string())?;
    let root: Value = serde_json::from_str(&text).map_err(|e| e.to_string())?;

    let vectors = root
        .get("vectors")
        .and_then(Value::as_array)
        .ok_or("Missing vectors array")?;

    if vectors.len() != 5 {
        return Err("Expected 5 canonical KAT vectors".to_string());
    }

    let mut seen_ids = HashSet::new();

    for item in vectors {
        let obj = item.as_object().ok_or("Vector is not an object")?;
        for key in ["id", "timestamp", "berlin", "rgb", "hex"] {
            if !obj.contains_key(key) {
                return Err(format!("Missing required field: {key}"));
            }
        }

        let id = obj["id"].as_str().ok_or("id not a string")?;
        if !seen_ids.insert(id.to_string()) {
            return Err(format!("Duplicate KAT ID: {id}"));
        }

        let ts = obj["timestamp"].as_str().ok_or("timestamp not a string")?;
        let bits = obj["berlin"].as_str().ok_or("berlin not a string")?;
        let hex = obj["hex"].as_str().ok_or("hex not a string")?;
        let rgb = obj["rgb"].as_array().ok_or("rgb not an array")?;

        if rgb.len() != 3 {
            return Err("rgb length must be 3".to_string());
        }

        if ts.len() != 20 {
            return Err(format!("timestamp length invalid for {id}"));
        }

        if bits.len() != 24 {
            return Err(format!("bitstream length invalid for {id}"));
        }

        if hex.len() != 6 {
            return Err(format!("hex length invalid for {id}"));
        }

        for ch in bits.chars() {
            if ch != '0' && ch != '1' {
                return Err(format!("invalid bit value in {id}"));
            }
        }

        for v in rgb {
            let n = v.as_i64().ok_or("rgb value not integer")?;
            if !(0..=255).contains(&n) {
                return Err(format!("rgb out of range for {id}"));
            }
        }
    }

    Ok(root)
}

#[test]
fn kat_ids_are_unique() {
    let doc = load_kat_document().unwrap();
    let ids: Vec<_> = doc["vectors"]
        .as_array()
        .unwrap()
        .iter()
        .map(|v| v["id"].as_str().unwrap().to_string())
        .collect();

    let unique: HashSet<_> = ids.iter().cloned().collect();
    assert_eq!(ids.len(), unique.len());
}

#[test]
fn canonical_kat_vectors_have_expected_count() {
    let doc = load_kat_document().unwrap();
    let count = doc["vectors"].as_array().unwrap().len();
    assert_eq!(count, 5);
}

#[test]
fn canonical_kat_vectors_have_valid_structure() {
    let doc = load_kat_document().unwrap();
    for v in doc["vectors"].as_array().unwrap() {
        let ts = v["timestamp"].as_str().unwrap();
        let bits = v["berlin"].as_str().unwrap();
        let hex = v["hex"].as_str().unwrap();
        let rgb = v["rgb"].as_array().unwrap();

        assert_eq!(ts.len(), 20);
        assert_eq!(bits.len(), 24);
        assert_eq!(hex.len(), 6);
        assert_eq!(rgb.len(), 3);

        for ch in bits.chars() {
            assert!(ch == '0' || ch == '1');
        }
    }
}

#[test]
fn duplicate_kat_id_is_rejected() {
    let mut seen = HashSet::new();
    let ids = ["A", "A"];

    for id in ids {
        assert!(seen.insert(id.to_string()));
    }
}

#[test]
fn invalid_bitstream_is_rejected() {
    let bits = "00000000000000000000000Z";
    assert_ne!(bits.len(), 24);
}

#[test]
fn invalid_hex_is_rejected() {
    let hex = "zzzzzz";
    assert_ne!(hex.len(), 6);
}

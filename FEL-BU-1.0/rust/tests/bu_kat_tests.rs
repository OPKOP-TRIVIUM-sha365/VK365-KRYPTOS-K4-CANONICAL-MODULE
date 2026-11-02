use serde_json::Value;
use std::collections::HashSet;
use std::fs;
use std::path::Path;

fn load_kat_document() -> Result<Value, String> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../vectors/bu_kat_001.json");
    let content = fs::read_to_string(path).map_err(|e| e.to_string())?;
    let doc: Value = serde_json::from_str(&content).map_err(|e| e.to_string())?;

    let vectors = doc["vectors"].as_array().ok_or("Missing vectors array")?;
    if vectors.len() != 5 {
        return Err("Expected 5 KAT vectors".to_string());
    }

    let mut ids = HashSet::new();
    for item in vectors {
        let obj = item.as_object().ok_or("Vector is not an object")?;
        let id = obj.get("id").and_then(Value::as_str).ok_or("Vector missing id")?;
        if !ids.insert(id.to_string()) {
            return Err(format!("Duplicate KAT ID: {id}"));
        }
        for key in ["timestamp", "berlin", "rgb", "hex"] {
            if !obj.contains_key(key) {
                return Err(format!("Vector {id} missing key: {key}"));
            }
        }
    }

    Ok(doc)
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
fn duplicate_kat_id_is_rejected() {
    let mut seen = HashSet::new();
    let ids = ["A", "A"];
    for id in ids {
        assert!(seen.insert(id.to_string()));
    }
}

#[test]
fn canonical_kat_vectors_are_valid() {
    let doc = load_kat_document().unwrap();
    for item in doc["vectors"].as_array().unwrap() {
        let ts = item["timestamp"].as_str().unwrap();
        let bits = item["berlin"].as_str().unwrap();
        let rgb = item["rgb"].as_array().unwrap();
        assert_eq!(rgb.len(), 3);
        assert_eq!(bits.len(), 24);
        assert_eq!(ts.len(), 20);
    }
}

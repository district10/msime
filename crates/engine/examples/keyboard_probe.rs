//! Ask the real dictionaries what a set of keystrokes produces, under each scheme
//! and helpcode setting a user can be in.
//!
//! The scheme unit tests run against a synthetic dictionary, which answers "the
//! parser split this syllable" but not "the dictionary has a word for it". A report
//! that some input does not work needs the second answer, and the second answer only
//! exists against the pinned dictionaries this probe takes.
//!
//! usage: keyboard_probe <verified-resources> [keys...]

use msime_engine::host::{prepare_options, EngineOptions, Session};

const QUANPIN: u8 = 0;
const SHUANGPIN: u8 = 1;
const PROFILES: [&str; 4] = ["xiaohe", "ziranma", "shoudao", "microsoft"];
const DEFAULT_INPUTS: [&str; 4] = ["vswf", "nihk", "wf", "qwerty"];

fn candidates(options: &EngineOptions, keys: &str) -> msime_engine::Result<Vec<String>> {
    let mut session = Session::new(options)?;
    for key in keys.bytes() {
        session.character(key, false)?;
    }
    Ok(session.snapshot()?.candidates.into_iter().take(5).collect())
}

fn report(label: &str, options: &EngineOptions, inputs: &[String]) -> msime_engine::Result<()> {
    println!("{label}");
    for input in inputs {
        println!("  {input:<10} {:?}", candidates(options, input)?);
    }
    println!();
    Ok(())
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut arguments = std::env::args_os().skip(1);
    let resources = std::fs::canonicalize(
        arguments
            .next()
            .ok_or("usage: keyboard_probe <verified-resources> [keys...]")?,
    )?;
    let given: Vec<String> = arguments
        .map(|value| value.to_string_lossy().into_owned())
        .collect();
    let inputs = if given.is_empty() {
        DEFAULT_INPUTS.map(str::to_owned).to_vec()
    } else {
        given
    };

    let temporary = tempfile::tempdir()?;
    let mut base = prepare_options(
        resources.to_str().unwrap(),
        temporary.path().join("user").to_str().unwrap(),
        temporary.path().join("cache").to_str().unwrap(),
        "keyboard-probe",
    )?;
    base.learning = false;
    base.scheme = SHUANGPIN;
    base.shuangpin_profile = 1; // ziranma
    println!(
        "scheme=shuangpin profile={} helpcode={} schema={} english_minimum_prefix={}\n",
        PROFILES[base.shuangpin_profile as usize],
        base.helpcode,
        base.helpcode_schema,
        base.english_minimum_prefix
    );
    report("[as configured]", &base, &inputs)?;

    // Every shuangpin profile, since a report of "some words will not type" is
    // usually about the profile rather than the input.
    for (profile, name) in PROFILES.iter().enumerate() {
        let mut options = base.clone();
        options.shuangpin_profile = profile as u8;
        report(&format!("[shuangpin profile={name}]"), &options, &inputs)?;
    }

    let mut quanpin = base.clone();
    quanpin.scheme = QUANPIN;
    report("[quanpin]", &quanpin, &inputs)?;

    let mut without_helpcode = base.clone();
    without_helpcode.helpcode = false;
    report("[helpcode off]", &without_helpcode, &inputs)?;

    // The mixed-English row is the one that depends on the prefix length, so the
    // probe says what a shorter prefix would find.
    let mut short_prefix = base.clone();
    short_prefix.english_minimum_prefix = 2;
    report("[english from two letters]", &short_prefix, &inputs)?;
    Ok(())
}

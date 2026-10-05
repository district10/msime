//! Point a preferences directory at a keyboard layout, and set the neighbouring
//! settings this project's 自然码双拼 setup wants. There is no settings control for
//! any of it yet, so this is the way in.
//!
//! A hand written document is not an option: several fields carry no serde default
//! (`candidate_page_size`, `scheme`, `chinese_punctuation`, `learning`), so a partial
//! document is rejected whole and the host runs on defaults. This builds the
//! document with the same type the host reads it with.
//!
//! usage: set_physical_keyboard <directory> system
//!        set_physical_keyboard <directory> input_source <input-source-id>
//!        set_physical_keyboard <directory> mapping <top> <home> <bottom> <punct>
//!                                                       [<shift-top> <shift-home> <shift-bottom> <shift-punct>]

use msime_client_core::preferences::{
    HelpcodeSchema, InputScheme, PhysicalKeyboardMode, PhysicalKeyboardRows, PreferencesStore,
    ShuangpinProfile,
};
use std::path::Path;

fn row(value: Option<&String>) -> String {
    value.cloned().unwrap_or_default()
}

fn main() {
    let arguments: Vec<String> = std::env::args().skip(1).collect();
    let usage = "usage: set_physical_keyboard <directory> system|input_source <id>|mapping <top> <home> <bottom> <punct> [<shift...>]";
    if arguments.len() < 2 {
        eprintln!("{usage}");
        std::process::exit(2);
    }
    let directory = &arguments[0];
    let mode = arguments[1].as_str();
    let store = PreferencesStore::new(Path::new(directory));
    let (revision, mut preferences) = match store.load() {
        Ok(snapshot) => (snapshot.revision, snapshot.preferences),
        Err(error) => {
            eprintln!("existing document unusable ({error}); rebuilding from defaults");
            let _ = std::fs::remove_file(Path::new(directory).join("preferences.json"));
            (0, Default::default())
        }
    };

    // 自然码双拼, and the switches that make it behave the way this setup expects.
    preferences.scheme = InputScheme::Shuangpin;
    preferences.shuangpin_profile = ShuangpinProfile::Ziranma;
    preferences.diagnostic_log.server = true;
    // The macOS settings page for 辅助码 was retired upstream, so it cannot be turned
    // off from the interface at all. Off is what a two-syllable input like `vswf`
    // wants; the schema stays ziranma so turning it back on lines up with the profile.
    preferences.shuangpin_helpcode.enabled = false;
    preferences.shuangpin_helpcode.schema = HelpcodeSchema::Ziranma;
    // Mixed English only offers a candidate once the prefix reaches this length, and
    // the default of 5 leaves ordinary short words with nothing.
    preferences.mixed_input.english = true;
    preferences.mixed_input.minimum_prefix = 2;
    // The paging shortcuts are named after characters and follow the layout, so the
    // shipped defaults are safe to leave on: the physical comma no longer pages.
    preferences.navigation.minus_equal = true;
    preferences.navigation.comma_period = true;
    preferences.navigation.page_up_down = true;
    preferences.navigation.tab = true;

    let keyboard = &mut preferences.physical_keyboard;
    match mode {
        "system" => {
            keyboard.mode = PhysicalKeyboardMode::System;
            keyboard.input_source.clear();
        }
        "input_source" => {
            let Some(identifier) = arguments.get(2) else {
                eprintln!("input_source needs an input source identifier\n{usage}");
                std::process::exit(2);
            };
            keyboard.mode = PhysicalKeyboardMode::InputSource;
            keyboard.input_source = identifier.clone();
        }
        "mapping" => {
            if arguments.len() < 6 {
                eprintln!("mapping needs four rows\n{usage}");
                std::process::exit(2);
            }
            keyboard.mode = PhysicalKeyboardMode::Mapping;
            keyboard.input_source.clear();
            keyboard.rows = PhysicalKeyboardRows {
                top: arguments[2].clone(),
                home: arguments[3].clone(),
                bottom: arguments[4].clone(),
                punct: arguments[5].clone(),
                shift_top: row(arguments.get(6)),
                shift_home: row(arguments.get(7)),
                shift_bottom: row(arguments.get(8)),
                shift_punct: row(arguments.get(9)),
            };
        }
        other => {
            eprintln!("unknown mode {other}\n{usage}");
            std::process::exit(2);
        }
    }

    if let Err(error) = preferences.validate() {
        eprintln!("refusing to write: {error}");
        std::process::exit(1);
    }
    match store.save(revision, preferences) {
        Ok(saved) => println!(
            "wrote revision {} with physical_keyboard = {:?}",
            saved.revision, saved.preferences.physical_keyboard
        ),
        Err(error) => {
            eprintln!("could not write: {error}");
            std::process::exit(1);
        }
    }
}

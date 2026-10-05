//! Report what a preferences directory actually loads to, the way the host loads
//! it. A document the host refuses leaves it running on defaults, which from the
//! outside looks like "the setting did nothing" - and the refusal itself is not
//! reported anywhere the user can see.
//!
//! usage: check_preferences <directory>

use msime_client_core::preferences::PreferencesStore;
use std::path::Path;

fn main() {
    let Some(directory) = std::env::args().nth(1) else {
        eprintln!("usage: check_preferences <directory>");
        std::process::exit(2);
    };
    match PreferencesStore::new(Path::new(&directory)).load() {
        Ok(snapshot) => {
            let preferences = &snapshot.preferences;
            println!("revision {}", snapshot.revision);
            println!(
                "  scheme {:?} / {:?}",
                preferences.scheme, preferences.shuangpin_profile
            );
            println!(
                "  last_chinese_scheme {:?}",
                preferences.last_chinese_scheme
            );
            println!("  shuangpin_helpcode {:#?}", preferences.shuangpin_helpcode);
            println!("  mixed_input {:#?}", preferences.mixed_input);
            println!("  navigation {:#?}", preferences.navigation);
            println!("  physical_keyboard {:#?}", preferences.physical_keyboard);
        }
        Err(error) => {
            println!("the host will refuse this document: {error}");
            std::process::exit(1);
        }
    }
}

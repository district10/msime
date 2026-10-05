// Reading physical keys through a layout the system is not using: the rows a
// document carries, and the data of an installed input source. Both have to land
// on the same characters, because they are two ways of naming one layout.
#include <cassert>
#include <string>

#include "../../src/input/PhysicalKeyboardLayout.h"

namespace {

using msime::mac::PhysicalKeyboardBottomKeys;
using msime::mac::PhysicalKeyboardHomeKeys;
using msime::mac::PhysicalKeyboardPunctKeys;
using msime::mac::PhysicalKeyboardRowCharacter;
using msime::mac::PhysicalKeyboardTopKeys;

// The characters a run of keys produces, in physical order.
std::string typed(const msime::mac::PhysicalKeyboardRows &rows, const unsigned short *keys, size_t count, bool shift) {
    std::string characters;
    for (size_t index = 0; index < count; ++index) characters += PhysicalKeyboardRowCharacter(rows, keys[index], shift);
    return characters;
}

// The same, through an installed layout's own data.
std::string typed(const msime::mac::PhysicalKeyboardInputSource &source, const unsigned short *keys, size_t count) {
    std::string characters;
    for (size_t index = 0; index < count; ++index) characters += source.character(keys[index], false);
    return characters;
}

// The four rows a Dvorak variant is written as: the layout this was built for,
// with F and G swapped against Dvorak and the right-hand punctuation carrying
// letters.
msime::mac::PhysicalKeyboardRows variant() {
    msime::mac::PhysicalKeyboardRows rows;
    rows.top = ";,.kyfgclz";
    rows.shiftTop = ":<>KYFGCLZ";
    rows.home = "aoeiudrts";
    rows.shiftHome = "AOEIUDRTS";
    rows.bottom = "pqjhxbm";
    rows.shiftBottom = "PQJHXBM";
    rows.punct = "n'wv/";
    rows.shiftPunct = "N\"WV?";
    return rows;
}

}  // namespace

int main() {
    const msime::mac::PhysicalKeyboardRows rows = variant();
    assert(typed(rows, PhysicalKeyboardTopKeys.data(), PhysicalKeyboardTopKeys.size(), false) == ";,.kyfgclz");
    assert(typed(rows, PhysicalKeyboardHomeKeys.data(), PhysicalKeyboardHomeKeys.size(), false) == "aoeiudrts");
    assert(typed(rows, PhysicalKeyboardBottomKeys.data(), PhysicalKeyboardBottomKeys.size(), false) == "pqjhxbm");
    assert(typed(rows, PhysicalKeyboardPunctKeys.data(), PhysicalKeyboardPunctKeys.size(), false) == "n'wv/");
    assert(typed(rows, PhysicalKeyboardTopKeys.data(), PhysicalKeyboardTopKeys.size(), true) == ":<>KYFGCLZ");
    assert(typed(rows, PhysicalKeyboardPunctKeys.data(), PhysicalKeyboardPunctKeys.size(), true) == "N\"WV?");

    // The digit row, the left-hand punctuation and the keypad belong to the
    // platform: answering '\0' is what makes the caller keep what it translated.
    for (unsigned short keyCode : {18, 19, 20, 21, 23, 22, 26, 28, 25, 29, 27, 24, 33, 30, 42, 50, 36, 51, 65, 67})
        assert(PhysicalKeyboardRowCharacter(rows, keyCode, false) == '\0');

    // The flat keyboard order and the four translation tables name the same keys. The
    // two orders differ - the chart interleaves the right-hand punctuation into the
    // rows it sits in, the translation keeps it as a row of its own - so they are
    // compared through the key code, which is what makes a chart drawn from one and
    // translated through the other line up.
    {
        std::string chart;
        for (const auto &key : msime::mac::PhysicalKeyboardKeys) chart += key.second;
        assert(chart == "QWERTYUIOPASDFGHJKL;'ZXCVBNM,./");
        const auto letter_of = [](unsigned short code) -> const char * {
            for (const auto &key : msime::mac::PhysicalKeyboardKeys)
                if (key.first == code) return key.second;
            return nullptr;
        };
        const std::string expected = "QWERTYUIOP" "ASDFGHJKL" "ZXCVBNM" ";',./";
        size_t letter = 0;
        const auto same_keys = [&](const auto &row) {
            for (const unsigned short code : row) {
                const char *face = letter_of(code);
                assert(face != nullptr);
                assert(face[0] == expected[letter]);
                ++letter;
            }
        };
        same_keys(msime::mac::PhysicalKeyboardTopKeys);
        same_keys(msime::mac::PhysicalKeyboardHomeKeys);
        same_keys(msime::mac::PhysicalKeyboardBottomKeys);
        same_keys(msime::mac::PhysicalKeyboardPunctKeys);
        assert(letter == expected.size());
        assert(letter == msime::mac::PhysicalKeyboardKeys.size());
        size_t counted = 0;
        for (const size_t count : msime::mac::PhysicalKeyboardKeyRowCounts) counted += count;
        assert(counted == msime::mac::PhysicalKeyboardKeys.size());
    }

    // Row membership by key code, including the two ends of the punctuation row.
    const msime::mac::PhysicalKeyboardPosition q = msime::mac::PhysicalKeyboardPositionOf(12);
    assert(q.row == 0 && q.index == 0);
    const msime::mac::PhysicalKeyboardPosition slash = msime::mac::PhysicalKeyboardPositionOf(44);
    assert(slash.row == 3 && slash.index == 4);
    assert(msime::mac::PhysicalKeyboardPositionOf(18).row == -1);

    // With no shifted row, a letter takes its uppercase and every other key
    // answers what it answers unshifted.
    msime::mac::PhysicalKeyboardRows letters;
    letters.top = "qwertyuiop";
    letters.punct = "n'wv/";
    assert(PhysicalKeyboardRowCharacter(letters, 12, true) == 'Q');
    assert(PhysicalKeyboardRowCharacter(letters, 44, true) == '/');

    // A document is hand-editable, so a row that does not cover its keys is refused
    // whole rather than applied to the first however many keys it names.
    assert(msime::mac::PhysicalKeyboardRowsFit(rows));
    assert(msime::mac::PhysicalKeyboardRowsFit(letters));
    for (const msime::mac::PhysicalKeyboardRows &broken : {
             msime::mac::PhysicalKeyboardRows{},                                  // nothing configured
             [] { msime::mac::PhysicalKeyboardRows r; r.top = "qwerty"; return r; }(),
             [] { msime::mac::PhysicalKeyboardRows r; r.top = "qwertyuiopa"; return r; }(),
             [] { msime::mac::PhysicalKeyboardRows r; r.punct = "n'wv"; return r; }(),
             [] { msime::mac::PhysicalKeyboardRows r; r.shiftHome = "ASDFGHJKL"; return r; }(),
             [] { msime::mac::PhysicalKeyboardRows r; r.top = "qwertyuiop"; r.punct = "n wv/"; return r; }(),
         })
        assert(!msime::mac::PhysicalKeyboardRowsFit(broken));
    assert(msime::mac::PhysicalKeyboardRowsFit(letters));

    // An installed layout answers the same characters as the rows above it, which
    // is the whole point of naming one: the characters come from the system's own
    // copy of the layout rather than from a second table here.
    msime::mac::PhysicalKeyboardInputSource dvorak;
    assert(dvorak.use("com.apple.keylayout.Dvorak"));
    assert(typed(dvorak, PhysicalKeyboardTopKeys.data(), PhysicalKeyboardTopKeys.size()) == "',.pyfgcrl");
    assert(typed(dvorak, PhysicalKeyboardHomeKeys.data(), PhysicalKeyboardHomeKeys.size()) == "aoeuidhtn");
    assert(dvorak.character(12, true) == '"');

    // An identifier no installed layout has leaves the layout inert rather than
    // throwing, so a user who uninstalled theirs keeps typing on the system
    // layout instead of losing the keyboard.
    msime::mac::PhysicalKeyboardInputSource missing;
    assert(!missing.use("org.unknown.keylayout.not-installed"));
    assert(!missing.ready());
    assert(missing.character(12, false) == '\0');
    assert(!missing.use(""));

    // Resolving a second layout drops the first.
    assert(dvorak.use("com.apple.keylayout.US"));
    assert(dvorak.character(12, false) == 'q');
    dvorak.reset();
    assert(!dvorak.ready());
    return 0;
}

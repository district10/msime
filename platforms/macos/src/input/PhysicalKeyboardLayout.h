#pragma once

//! Reading a physical key through a keyboard layout the system is not using.
//!
//! The input method normally types whatever the platform translated the key into,
//! which is the layout the user selected in System Settings. This lets it type
//! through a *named* layout instead, so that the layout applies here and nowhere
//! else: switching the system layout is global, and a private layout a user made
//! in Ukelele is not something they can select for one application.
//!
//! Two ways to name one. `PhysicalKeyboardInputSource` resolves an installed
//! layout by its identifier and translates through that layout's own data, so the
//! characters come from the layout itself and nothing is copied here.
//! `PhysicalKeyboardRows` carries the layout inline instead, as the characters
//! each key produces, for a machine the layout is not installed on.
//!
//! Only the letter block and the right-hand punctuation are covered: the digit
//! row, the left-hand punctuation, the keypad and the keys a JIS keyboard adds
//! keep the platform's characters. That is deliberate - a layout that wants one of
//! those can only come from the platform's own data, and returning "no character"
//! for them is what makes the caller fall back rather than type nothing.

#include <Carbon/Carbon.h>

#include <array>
#include <string>
#include <string_view>
#include <utility>

namespace msime::mac {

/// The four configured rows of a layout, in physical order. An empty row leaves
/// that run of keys to the platform.
struct PhysicalKeyboardRows {
    std::string top;
    std::string home;
    std::string bottom;
    std::string punct;
    std::string shiftTop;
    std::string shiftHome;
    std::string shiftBottom;
    std::string shiftPunct;

    bool empty() const { return top.empty() && home.empty() && bottom.empty() && punct.empty(); }
};

/// The physical keys each row covers, as macOS virtual key codes. Every one of
/// them sits at the same physical place on an ANSI and on a JIS keyboard, so one
/// table covers both. The keys a JIS keyboard adds - `kVK_JIS_Yen`,
/// `kVK_JIS_Underscore` - are not in any row, which is also where a layout of this
/// kind leaves them.
inline constexpr std::array<unsigned short, 10> PhysicalKeyboardTopKeys{
    12, 13, 14, 15, 17, 16, 32, 34, 31, 35};  // Q W E R T Y U I O P
inline constexpr std::array<unsigned short, 9> PhysicalKeyboardHomeKeys{
    0, 1, 2, 3, 5, 4, 38, 40, 37};  // A S D F G H J K L
inline constexpr std::array<unsigned short, 7> PhysicalKeyboardBottomKeys{
    6, 7, 8, 9, 11, 45, 46};  // Z X C V B N M
inline constexpr std::array<unsigned short, 5> PhysicalKeyboardPunctKeys{
    41, 39, 43, 47, 44};  // ; ' , . /

/// The physical keys a keyboard layout can move, in keyboard order, as the key code
/// and the ANSI letter printed on that position. The four row tables above are these
/// same keys grouped for the translation; this is the flat order a keyboard picture
/// is drawn in, so the shuangpin keymap and the host resolve the same positions.
///
/// The right-hand punctuation is part of it because a layout may put a letter there:
/// the Dvorak family does, which is why a keymap drawn from the ANSI letters alone
/// loses those letters for anyone typing on one.
inline constexpr std::array<std::pair<unsigned short, const char *>, 31> PhysicalKeyboardKeys{{
    {12, "Q"}, {13, "W"}, {14, "E"}, {15, "R"}, {17, "T"}, {16, "Y"},
    {32, "U"}, {34, "I"}, {31, "O"}, {35, "P"},
    {0, "A"}, {1, "S"}, {2, "D"}, {3, "F"}, {5, "G"},
    {4, "H"}, {38, "J"}, {40, "K"}, {37, "L"}, {41, ";"}, {39, "'"},
    {6, "Z"}, {7, "X"}, {8, "C"}, {9, "V"}, {11, "B"}, {45, "N"}, {46, "M"},
    {43, ","}, {47, "."}, {44, "/"},
}};

/// How many of [`PhysicalKeyboardKeys`] each row holds, top to bottom.
inline constexpr std::array<size_t, 3> PhysicalKeyboardKeyRowCounts{10, 11, 10};

/// Where a key code sits: which row, and how far into it. A key no row covers
/// answers a row of -1, which is the digit row, the left-hand punctuation and
/// everything outside the letter block.
struct PhysicalKeyboardPosition {
    int row;
    int index;
};

inline PhysicalKeyboardPosition PhysicalKeyboardPositionOf(unsigned short keyCode) {
    constexpr std::array<const unsigned short *, 4> rows{PhysicalKeyboardTopKeys.data(),
                                                         PhysicalKeyboardHomeKeys.data(),
                                                         PhysicalKeyboardBottomKeys.data(),
                                                         PhysicalKeyboardPunctKeys.data()};
    constexpr std::array<size_t, 4> counts{PhysicalKeyboardTopKeys.size(), PhysicalKeyboardHomeKeys.size(),
                                           PhysicalKeyboardBottomKeys.size(), PhysicalKeyboardPunctKeys.size()};
    for (size_t row = 0; row < rows.size(); ++row)
        for (size_t index = 0; index < counts[row]; ++index)
            if (rows[row][index] == keyCode) return {static_cast<int>(row), static_cast<int>(index)};
    return {-1, -1};
}

/// How many keys each row covers, in the order [`PhysicalKeyboardRows`] declares
/// them: top, home, bottom, punct.
inline constexpr std::array<size_t, 4> PhysicalKeyboardRowKeys{PhysicalKeyboardTopKeys.size(),
                                                               PhysicalKeyboardHomeKeys.size(),
                                                               PhysicalKeyboardBottomKeys.size(),
                                                               PhysicalKeyboardPunctKeys.size()};

/// Whether a parsed layout is one to read keys through: every row either absent
/// or exactly as long as the keys it covers, and a shifted row only beside the
/// row it shifts.
///
/// The shared document validates this too, but a layout is also something a user
/// edits by hand - the setting has no control yet - and the file reaches this host
/// without passing through that check. A row that does not fit is refused whole
/// rather than applied to the first however many keys it happens to name.
inline bool PhysicalKeyboardRowsFit(const PhysicalKeyboardRows &rows) {
    const std::array<const std::string *, 4> plain{&rows.top, &rows.home, &rows.bottom, &rows.punct};
    const std::array<const std::string *, 4> shifted{&rows.shiftTop, &rows.shiftHome, &rows.shiftBottom,
                                                     &rows.shiftPunct};
    for (size_t row = 0; row < plain.size(); ++row) {
        for (const std::string *text : {plain[row], shifted[row]}) {
            if (text->empty()) continue;
            if (text->size() != PhysicalKeyboardRowKeys[row]) return false;
            for (const char character : *text)
                if (static_cast<unsigned char>(character) < 0x21 ||
                    static_cast<unsigned char>(character) > 0x7e)
                    return false;
        }
        // A shifted row describes keys its unshifted row has to name first.
        if (!shifted[row]->empty() && plain[row]->empty()) return false;
    }
    return !rows.empty();
}

/// The character a configured layout puts on a key, or '\0' for a key it does not
/// cover, which the caller answers with the platform's own character.
///
/// `shifted` is the row's Shift counterpart. An empty one leaves Shift to the rule
/// the rows can state by themselves: a letter becomes its uppercase, and every
/// other key answers what it answers unshifted.
inline char PhysicalKeyboardRowCharacter(const PhysicalKeyboardRows &rows, unsigned short keyCode, bool shift) {
    const PhysicalKeyboardPosition position = PhysicalKeyboardPositionOf(keyCode);
    if (position.row < 0) return '\0';
    const std::array<const std::string *, 4> plain{&rows.top, &rows.home, &rows.bottom, &rows.punct};
    const std::array<const std::string *, 4> shifted{&rows.shiftTop, &rows.shiftHome, &rows.shiftBottom,
                                                     &rows.shiftPunct};
    const std::string &row = *plain[position.row];
    const size_t index = static_cast<size_t>(position.index);
    if (row.empty() || index >= row.size()) return '\0';
    const char character = row[index];
    if (!shift) return character;
    const std::string &shiftRow = *shifted[position.row];
    if (!shiftRow.empty()) return index < shiftRow.size() ? shiftRow[index] : character;
    return character >= 'a' && character <= 'z' ? static_cast<char>(character - 'a' + 'A') : character;
}

/// One installed keyboard layout's own data, resolved by identifier and kept so
/// that a keystroke does not search the input source list again.
class PhysicalKeyboardInputSource {
  public:
    PhysicalKeyboardInputSource() = default;
    ~PhysicalKeyboardInputSource() { reset(); }
    PhysicalKeyboardInputSource(const PhysicalKeyboardInputSource &) = delete;
    PhysicalKeyboardInputSource &operator=(const PhysicalKeyboardInputSource &) = delete;

    /// Resolve the layout with this identifier. False leaves the object inert, so
    /// typing is unchanged; a layout the user uninstalled therefore degrades to
    /// the system layout rather than to no input.
    bool use(std::string_view identifier) {
        reset();
        if (identifier.empty()) return false;
        CFStringRef wanted = CFStringCreateWithBytes(kCFAllocatorDefault,
                                                     reinterpret_cast<const UInt8 *>(identifier.data()),
                                                     static_cast<CFIndex>(identifier.size()),
                                                     kCFStringEncodingUTF8, false);
        if (!wanted) return false;
        CFArrayRef sources = TISCreateInputSourceList(nullptr, true);
        if (sources) {
            const CFIndex count = CFArrayGetCount(sources);
            for (CFIndex index = 0; index < count && !layout_; ++index) {
                TISInputSourceRef source = static_cast<TISInputSourceRef>(
                    const_cast<void *>(CFArrayGetValueAtIndex(sources, index)));
                CFStringRef identifierProperty =
                    static_cast<CFStringRef>(TISGetInputSourceProperty(source, kTISPropertyInputSourceID));
                if (!identifierProperty || CFStringCompare(identifierProperty, wanted, 0) != kCFCompareEqualTo)
                    continue;
                void *data = TISGetInputSourceProperty(source, kTISPropertyUnicodeKeyLayoutData);
                if (!data) continue;
                data_ = static_cast<CFDataRef>(CFRetain(static_cast<CFTypeRef>(data)));
                layout_ = reinterpret_cast<const UCKeyboardLayout *>(CFDataGetBytePtr(data_));
            }
            CFRelease(sources);
        }
        CFRelease(wanted);
        return layout_ != nullptr;
    }

    void reset() {
        if (data_) CFRelease(data_);
        data_ = nullptr;
        layout_ = nullptr;
    }

    bool ready() const { return layout_ != nullptr; }

    /// The character this physical key produces under the layout, or '\0' when it
    /// produces none (a dead key, a modifier-only key, or a key the layout leaves
    /// alone), which the caller answers with the platform's own character.
    char character(unsigned short keyCode, bool shift) const {
        if (!layout_) return '\0';
        // UCKeyTranslate wants the Carbon modifier state in its high byte. Shift
        // and Caps Lock are the two this reads: the callers have already excluded
        // Command, Control and Option, which belong to shortcuts rather than to
        // typing.
        UInt32 modifiers = 0;
        if (shift) modifiers |= (shiftKey >> 8);
        UInt32 deadKeys = 0;
        UniChar buffer[4] = {};
        UniCharCount length = 0;
        const OSStatus status = UCKeyTranslate(layout_, keyCode, kUCKeyActionDown, modifiers, LMGetKbdType(),
                                               kUCKeyTranslateNoDeadKeysBit, &deadKeys, 4, &length, buffer);
        if (status != noErr || length != 1 || buffer[0] > 0x7f) return '\0';
        return static_cast<char>(buffer[0]);
    }

  private:
    CFDataRef data_ = nullptr;
    const UCKeyboardLayout *layout_ = nullptr;
};

}  // namespace msime::mac

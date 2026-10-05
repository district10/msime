#pragma once

#import <AppKit/AppKit.h>

FOUNDATION_EXPORT NSArray<NSArray<NSDictionary<NSString *, NSString *> *> *> *MSIMEShuangpinKeymapRows(
    NSString *profileName);

/// The same rows, with each key's face taken from the character a configured
/// keyboard layout puts on that physical position instead of the position's own
/// ANSI letter, and the hints looked up for that character. The chart pictures the
/// keyboard the user types on, so a layout that moves the letters - the Dvorak
/// family puts `w` and `v` on the right-hand punctuation - has to move the chart
/// with them. An empty map draws the ANSI letters, which is what someone typing on
/// the layout the system is set to sees.
FOUNDATION_EXPORT NSArray<NSArray<NSDictionary<NSString *, NSString *> *> *> *
MSIMEShuangpinKeymapRowsForCharacters(NSString *profileName,
                                      NSDictionary<NSString *, NSString *> *characters);

FOUNDATION_EXPORT NSString *MSIMEShuangpinZeroInitialText(NSString *profileName);

FOUNDATION_EXPORT BOOL MSIMEShouldShowShuangpinKeymap(BOOL isShuangpin, BOOL enabled, BOOL hasComposition);

// Raw Engine input, independent of the selected preedit display format.
FOUNDATION_EXPORT NSString *MSIMEShuangpinKeymapEditingText(NSDictionary *view);
FOUNDATION_EXPORT NSString *MSIMEShuangpinKeymapHighlightedKey(NSDictionary *view);

FOUNDATION_EXPORT NSRect MSIMEShuangpinKeymapPanelFrame(NSRect caretRect, NSSize panelSize,
                                                              CGFloat candidateClearance, NSRect visibleFrame);

@interface MSIMEShuangpinKeymapPanel : NSPanel
- (void)setProfileName:(NSString *)profileName;
/// The character each physical position types under the configured keyboard layout,
/// keyed by the ANSI letter printed on that position. An empty dictionary draws the
/// ANSI letters.
- (void)setPhysicalKeyboardCharacters:(NSDictionary<NSString *, NSString *> *)characters;
- (void)updateHighlightedKey:(NSString *)key;
/// The fill of the highlighted key: the theme's accent, as a dynamic colour so it resolves in the panel's appearance. Until set, the panel's own teal.
- (void)setAccentColor:(NSColor *)accent;
@property(nonatomic, readonly) NSColor *accentColor;
- (void)showNearCaretRect:(NSRect)caretRect candidateClearance:(CGFloat)candidateClearance;
@end

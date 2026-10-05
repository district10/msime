#import "../../src/settings/ShuangpinKeymapPanel.h"
#include <cassert>

int main() {
    @autoreleasepool {
        [NSApplication sharedApplication];
        MSIMEShuangpinKeymapPanel *panel = [MSIMEShuangpinKeymapPanel new];
        panel.releasedWhenClosed = NO;
        [panel setProfileName:@"xiaohe"];
        for (NSString *preedit in @[@"hk", @"hao", @"hao ", @""]) {
            NSDictionary *view = @{@"editing_text": @"hk", @"preedit": preedit};
            assert([MSIMEShuangpinKeymapEditingText(view) isEqual:@"hk"]);
            assert([MSIMEShuangpinKeymapHighlightedKey(view) isEqual:@"k"]);
            [panel updateHighlightedKey:MSIMEShuangpinKeymapHighlightedKey(view)];
            assert([panel.contentView.accessibilityValue containsString:@"当前按键 K"]);
        }
        [panel setProfileName:@"microsoft"];
        NSDictionary *semicolon = @{@"editing_text": @"b;", @"preedit": @"bing"};
        [panel updateHighlightedKey:MSIMEShuangpinKeymapHighlightedKey(semicolon)];
        assert([panel.contentView.accessibilityValue containsString:@"当前按键 ;"]);
        assert([MSIMEShuangpinKeymapHighlightedKey(@{@"editing_text": @"H"}) isEqual:@"H"]);
        for (id raw in @[@"", NSNull.null, @42, @[]]) {
            NSDictionary *view = @{@"editing_text": raw, @"preedit": @"hao"};
            assert(MSIMEShuangpinKeymapEditingText(view).length == 0);
            assert(!MSIMEShouldShowShuangpinKeymap(YES, YES, MSIMEShuangpinKeymapEditingText(view).length > 0));
            assert(MSIMEShuangpinKeymapHighlightedKey(view).length == 0);
        }
        assert(MSIMEShuangpinKeymapEditingText(nil).length == 0);
        assert(MSIMEShuangpinKeymapHighlightedKey(@{@"preedit": @"hao"}).length == 0);
        for (NSString *raw in @[@"hk1", @"hk'", @"hk ", @"🙂"]) {
            [panel updateHighlightedKey:MSIMEShuangpinKeymapHighlightedKey(@{@"editing_text": raw})];
            assert(![panel.contentView.accessibilityValue containsString:@"当前按键"]);
        }
        // The highlighted key takes the theme accent the controller hands over, and keeps it across profile changes.
        assert(panel.accentColor != nil);
        NSColor *accent = [NSColor colorWithSRGBRed:0.36 green:0.61 blue:1.0 alpha:1.0];
        [panel setAccentColor:accent];
        assert([panel.accentColor isEqual:accent]);
        [panel setProfileName:@"ziranma"];
        assert([panel.accentColor isEqual:accent]);

        // The chart's faces follow the configured keyboard layout: a physical key is
        // drawn as the character it types there, with that character's hint. An empty
        // map draws the ANSI letters, which is the chart as it always was.
        assert([MSIMEShuangpinKeymapRows(@"ziranma") isEqual:MSIMEShuangpinKeymapRowsForCharacters(@"ziranma", @{})]);
        assert([MSIMEShuangpinKeymapRows(@"ziranma")[0][0][@"key"] isEqual:@"Q"]);
        // The Dvorak family types `w` and `v` from the right-hand punctuation, so a
        // chart drawn from the ANSI letters alone loses those two finals entirely.
        // They are the reason the chart covers those positions at all.
        NSDictionary<NSString *, NSString *> *dvorakMod = @{
            @"Q": @";", @"W": @",", @"E": @".", @"R": @"k", @"T": @"y", @"Y": @"f",
            @"U": @"g", @"I": @"c", @"O": @"l", @"P": @"z",
            @"A": @"a", @"S": @"o", @"D": @"e", @"F": @"i", @"G": @"u", @"H": @"d",
            @"J": @"r", @"K": @"t", @"L": @"s", @";": @"n", @"'": @"'",
            @"Z": @"p", @"X": @"q", @"C": @"j", @"V": @"h", @"B": @"x", @"N": @"b",
            @"M": @"m", @",": @"w", @".": @"v", @"/": @"/",
        };
        NSArray *laid = MSIMEShuangpinKeymapRowsForCharacters(@"ziranma", dvorakMod);
        assert(![laid[0][0][@"key"] isEqual:@"Q"]);
        NSArray<NSString *> *expectedBottom = @[ @"P", @"Q", @"J", @"H", @"X", @"B", @"M", @"W", @"V" ];
        NSArray<NSString *> *bottomFaces = [laid[2] valueForKey:@"key"];
        assert([bottomFaces isEqual:expectedBottom]);
        NSArray<NSString *> *faces = [laid valueForKeyPath:@"@unionOfArrays.key"];
        for (NSString *expected in @[ @"A", @"O", @"E", @"I", @"U", @"D", @"R", @"T", @"S", @"N", @"W", @"V" ])
            assert([faces containsObject:expected]);
        // The panel takes the map without rebuilding what it has not been told about.
        [panel setPhysicalKeyboardCharacters:dvorakMod];
        [panel setProfileName:@"xiaohe"];
        [panel updateHighlightedKey:@"k"];
        assert([panel.contentView.accessibilityValue containsString:@"当前按键 K"]);
        [panel close];
    }
}

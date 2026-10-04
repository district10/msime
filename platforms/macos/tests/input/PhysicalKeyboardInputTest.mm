// The configured keyboard layout on the real controller: which character a
// physical key is read as, which keys it leaves alone, and what a shortcut keeps.
#import "../../src/input/InputController.mm"

#include <cassert>

// The layout this feature was built for, written the way a document carries it: a
// Dvorak variant with F and G swapped against Dvorak, and the right-hand
// punctuation carrying letters.
static NSDictionary *VariantLayout(void) {
    return @{
        @"mode" : @"mapping",
        @"rows" : @{
            @"top" : @";,.kyfgclz",
            @"shift_top" : @":<>KYFGCLZ",
            @"home" : @"aoeiudrts",
            @"shift_home" : @"AOEIUDRTS",
            @"bottom" : @"pqjhxbm",
            @"shift_bottom" : @"PQJHXBM",
            @"punct" : @"n'wv/",
            @"shift_punct" : @"N\"WV?",
        },
    };
}

// A physical key as AppKit delivers it. `characters` is what the *system* layout
// produced, which is exactly what the configured layout replaces.
static NSEvent *Physical(unsigned short code, NSString *systemCharacters, NSEventModifierFlags flags) {
    return [NSEvent keyEventWithType:NSEventTypeKeyDown
                            location:NSZeroPoint
                      modifierFlags:flags
                          timestamp:0
                       windowNumber:0
                            context:nil
                         characters:systemCharacters
      charactersIgnoringModifiers:systemCharacters
                          isARepeat:NO
                            keyCode:code];
}

// ensureAppearance would load the real preferences and start the Engine; the
// question here is only what a key is read as.
@interface LayoutController : MSIMEInputController
@end
@implementation LayoutController
- (void)ensureAppearance {}
@end

static NSString *Typed(LayoutController *controller, unsigned short code, NSString *system,
                       NSEventModifierFlags flags) {
    return [controller typedCharactersForEvent:Physical(code, system, flags) ignoringModifiers:NO];
}

int main() {
    @autoreleasepool {
        [NSApplication sharedApplication];
        LayoutController *controller = [LayoutController alloc];

        // Nothing configured: the platform's own translation stands, which is what
        // every key did before this existed. The system characters below are what a
        // Dvorak *system* layout produces, so they are deliberately unlike the
        // physical legends - a layout that is not configured must not touch them.
        [controller syncPhysicalKeyboardFromPreferences:@{}];
        assert([Typed(controller, 12, @"'", 0) isEqual:@"'"]);
        assert([Typed(controller, 41, @"s", 0) isEqual:@"s"]);

        // The rows, read at the physical positions they name.
        [controller syncPhysicalKeyboardFromPreferences:@{@"physical_keyboard" : VariantLayout()}];
        assert([Typed(controller, 12, @"'", 0) isEqual:@";"]);   // top[0]
        assert([Typed(controller, 35, @"l", 0) isEqual:@"z"]);   // top[9]
        assert([Typed(controller, 0, @"a", 0) isEqual:@"a"]);    // home[0]
        assert([Typed(controller, 37, @"n", 0) isEqual:@"s"]);   // home[8]
        assert([Typed(controller, 6, @"x", 0) isEqual:@"p"]);    // bottom[0]
        assert([Typed(controller, 46, @"m", 0) isEqual:@"m"]);   // bottom[6]
        assert([Typed(controller, 41, @"s", 0) isEqual:@"n"]);   // punct[0]: n on the physical ; key
        assert([Typed(controller, 44, @"z", 0) isEqual:@"/"]);   // punct[4]

        // Shift reads the shifted row, so the marks a layout moves come out right
        // rather than as the unshifted ones.
        assert([Typed(controller, 12, @"\"", NSEventModifierFlagShift) isEqual:@":"]);
        assert([Typed(controller, 41, @"S", NSEventModifierFlagShift) isEqual:@"N"]);
        assert([Typed(controller, 44, @"Z", NSEventModifierFlagShift) isEqual:@"?"]);

        // A key no row covers keeps what the system produced: the digit row, the
        // left-hand punctuation, Space, and the keys a JIS keyboard adds.
        for (NSNumber *code in @[ @18, @19, @24, @27, @33, @30, @49, @94, @93 ])
            assert([Typed(controller, (unsigned short)code.unsignedShortValue, @"1", 0) isEqual:@"1"]);

        // A chord belongs to the application, so it keeps the platform's
        // translation and never the layout's.
        for (NSNumber *modifier in @[ @(NSEventModifierFlagCommand), @(NSEventModifierFlagControl),
                                      @(NSEventModifierFlagOption) ])
            assert([Typed(controller, 12, @"q", (NSEventModifierFlags)modifier.unsignedIntegerValue) isEqual:@"q"]);

        // Rows that do not fit the keys they cover, or a mode with nothing to read,
        // leave typing exactly as it was rather than translating half a layout.
        [controller syncPhysicalKeyboardFromPreferences:@{@"physical_keyboard" : VariantLayout()}];
        for (NSDictionary *broken in @[
                 @{@"mode" : @"mapping", @"rows" : @{@"top" : @"qwerty"}},
                 @{@"mode" : @"input_source", @"input_source" : @"org.unknown.keylayout.not-installed"},
                 @{@"mode" : @"input_source"},
             ]) {
            [controller syncPhysicalKeyboardFromPreferences:@{@"physical_keyboard" : broken}];
            assert([Typed(controller, 12, @"'", 0) isEqual:@"'"]);
        }

        // An installed layout is named rather than copied, and answers the same
        // characters its own data holds. Any macOS has a US layout and Dvorak.
        [controller
            syncPhysicalKeyboardFromPreferences:@{@"physical_keyboard" : @{
                @"mode" : @"input_source", @"input_source" : @"com.apple.keylayout.Dvorak"}}];
        assert([Typed(controller, 12, @"'", 0) isEqual:@"'"]);   // Dvorak's top row starts ',.
        assert([Typed(controller, 41, @"s", 0) isEqual:@"s"]);
        [controller
            syncPhysicalKeyboardFromPreferences:@{@"physical_keyboard" : @{
                @"mode" : @"input_source", @"input_source" : @"com.apple.keylayout.US"}}];
        assert([Typed(controller, 12, @"x", 0) isEqual:@"q"]);

        // Back to the platform: a layout that is configured but not selected is not
        // read, and the mode decides rather than the presence of rows.
        [controller
            syncPhysicalKeyboardFromPreferences:@{@"physical_keyboard" : @{
                @"mode" : @"system", @"rows" : VariantLayout()[@"rows"]}}];
        assert([Typed(controller, 12, @"'", 0) isEqual:@"'"]);
    }
    return 0;
}

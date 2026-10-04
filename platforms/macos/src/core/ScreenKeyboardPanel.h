#import <AppKit/AppKit.h>
#include <sys/types.h>

typedef BOOL (^MSIMEScreenKeyboardSender)(unsigned short keyCode, NSEventModifierFlags flags);
typedef pid_t (^MSIMEScreenKeyboardTargetProvider)(void);

// The panel never becomes the input target. Tests supply a sender without posting events.
@interface MSIMEScreenKeyboardPanel : NSPanel
+ (instancetype)sharedPanel;
- (instancetype)initWithKeySender:(MSIMEScreenKeyboardSender)sender;
- (instancetype)initWithKeySender:(MSIMEScreenKeyboardSender)sender
                    targetProvider:(MSIMEScreenKeyboardTargetProvider)targetProvider;
- (void)showKeyboard;
- (void)applyThemePreferences:(NSDictionary *)preferences;
/// The characters the configured keyboard layout puts on the physical keys, by
/// NSNumber key code to a pair of (unshifted, shifted) strings. An empty
/// dictionary restores the table's own QWERTY legends, which is what a host with
/// no layout configured shows. The panel posts physical key codes, so without
/// this its faces would name keys the layout no longer types.
- (void)applyKeyboardLabels:(NSDictionary<NSNumber *, NSArray<NSString *> *> *)labels;
@end

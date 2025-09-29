#import "FlutterQrPlusPlugin.h"

#if __has_include(<qr_code_scanner_plus/qr_code_scanner_plus-Swift.h>)
#import <qr_code_scanner_plus/qr_code_scanner_plus-Swift.h>
#else
#import "qr_code_scanner_plus-Swift.h"
#endif

@implementation FlutterQrPlusPlugin
+ (void)registerWithRegistrar:(NSObject<FlutterPluginRegistrar>*)registrar {
    [SwiftFlutterQrPlusPlugin registerWithRegistrar:registrar];
}
@end

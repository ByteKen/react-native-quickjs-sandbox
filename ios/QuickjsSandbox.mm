#import "QuickjsSandbox.h"
#import "quickjs-sandbox-jsi.h"

@implementation QuickjsSandbox
- (NSNumber *)multiply:(double)a b:(double)b {
    NSNumber *result = @(a * b);
    return result;
}

- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params
{
    return std::make_shared<facebook::react::NativeQuickjsSandboxSpecJSI>(params);
}

+ (NSString *)moduleName
{
  return @"QuickjsSandbox";
}

#pragma mark - RCTTurboModuleWithJSIBindings

- (void)installJSIBindingsWithRuntime:(facebook::jsi::Runtime &)runtime
                           callInvoker:(const std::shared_ptr<facebook::react::CallInvoker> &)callinvoker
{
  quickjssandbox::installQuickjsSandbox(runtime);
}

@end

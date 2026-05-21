#include <jni.h>
#include <jsi/jsi.h>
#include "quickjs-sandbox-jsi.h"

extern "C" {

JNIEXPORT void JNICALL
Java_com_quickjssandbox_QuickjsSandboxModule_nativeInstall(JNIEnv *env, jobject thiz, jlong jsContextPointer) {
    auto *rt = reinterpret_cast<facebook::jsi::Runtime *>(jsContextPointer);
    if (rt) {
        quickjssandbox::installQuickjsSandbox(*rt);
    }
}

}

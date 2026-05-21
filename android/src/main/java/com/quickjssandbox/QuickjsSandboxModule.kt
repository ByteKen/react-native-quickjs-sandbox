package com.quickjssandbox

import com.facebook.react.bridge.ReactApplicationContext

class QuickjsSandboxModule(reactContext: ReactApplicationContext) :
  NativeQuickjsSandboxSpec(reactContext) {

  override fun multiply(a: Double, b: Double): Double {
    return a * b
  }

  override fun initialize() {
    super.initialize()
    try {
      System.loadLibrary("quickjs-sandbox-jsi")
      val jsContextPointer = reactApplicationContext.javaScriptContextHolder?.get() ?: 0L
      if (jsContextPointer != 0L) {
        nativeInstall(jsContextPointer)
      }
    } catch (e: Exception) {
      e.printStackTrace()
    }
  }

  private external fun nativeInstall(jsContextPointer: Long)

  companion object {
    const val NAME = NativeQuickjsSandboxSpec.NAME
  }
}

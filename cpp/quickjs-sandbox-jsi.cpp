#include "quickjs-sandbox-jsi.h"
#include "quickjs/quickjs.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace quickjssandbox {

struct SandboxContext {
    std::string contextId;
    JSRuntime *qjsRuntime = nullptr;
    JSContext *qjsContext = nullptr;
    facebook::jsi::Runtime *hermesRuntime = nullptr;
    std::shared_ptr<facebook::jsi::Function> hermesCallback = nullptr;

    ~SandboxContext() {
        if (qjsContext) {
            JS_FreeContext(qjsContext);
        }
        if (qjsRuntime) {
            JS_FreeRuntime(qjsRuntime);
        }
    }
};

static std::unordered_map<std::string, std::unique_ptr<SandboxContext>> activeContexts;
static std::mutex activeContextsMutex;

static JSValue js_native_call(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    SandboxContext *opaque = (SandboxContext *)JS_GetContextOpaque(ctx);
    if (!opaque || !opaque->hermesRuntime || !opaque->hermesCallback) {
        return JS_EXCEPTION;
    }

    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "nativeCall requires 2 arguments: action and argsJson");
    }

    const char *actionStr = JS_ToCString(ctx, argv[0]);
    const char *argsJsonStr = JS_ToCString(ctx, argv[1]);

    if (!actionStr || !argsJsonStr) {
        if (actionStr) JS_FreeCString(ctx, actionStr);
        if (argsJsonStr) JS_FreeCString(ctx, argsJsonStr);
        return JS_EXCEPTION;
    }

    std::string action(actionStr);
    std::string argsJson(argsJsonStr);

    JS_FreeCString(ctx, actionStr);
    JS_FreeCString(ctx, argsJsonStr);

    try {
        facebook::jsi::Runtime &rt = *(opaque->hermesRuntime);
        facebook::jsi::Value resVal = opaque->hermesCallback->call(
            rt,
            facebook::jsi::String::createFromUtf8(rt, opaque->contextId),
            facebook::jsi::String::createFromUtf8(rt, action),
            facebook::jsi::String::createFromUtf8(rt, argsJson)
        );

        if (resVal.isString()) {
            std::string resStr = resVal.getString(rt).utf8(rt);
            return JS_NewString(ctx, resStr.c_str());
        } else {
            return JS_UNDEFINED;
        }
    } catch (const std::exception &e) {
        return JS_ThrowInternalError(ctx, "Hermes callback crashed: %s", e.what());
    } catch (...) {
        return JS_ThrowInternalError(ctx, "Hermes callback crashed with unknown error");
    }
}

void installQuickjsSandbox(facebook::jsi::Runtime &rt) {
    // Clear any previous contexts to prevent memory leaks across JS reloads
    {
        std::lock_guard<std::mutex> lock(activeContextsMutex);
        activeContexts.clear();
    }

    auto sandboxObj = facebook::jsi::Object(rt);

    // createContext(contextId: string): boolean
    sandboxObj.setProperty(rt, "createContext", facebook::jsi::Function::createFromHostFunction(
        rt,
        facebook::jsi::PropNameID::forAscii(rt, "createContext"),
        1,
        [](facebook::jsi::Runtime &rt, const facebook::jsi::Value &thisVal, const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value {
            if (count < 1 || !args[0].isString()) {
                throw facebook::jsi::JSError(rt, "createContext requires a string contextId");
            }
            std::string contextId = args[0].getString(rt).utf8(rt);

            std::lock_guard<std::mutex> lock(activeContextsMutex);
            auto it = activeContexts.find(contextId);
            if (it != activeContexts.end()) {
                // If it already exists, erase/dispose the old context to avoid dangling callbacks on JS reload
                activeContexts.erase(it);
            }

            auto sandbox = std::make_unique<SandboxContext>();
            sandbox->contextId = contextId;
            sandbox->qjsRuntime = JS_NewRuntime();
            if (!sandbox->qjsRuntime) {
                throw facebook::jsi::JSError(rt, "Failed to create QuickJS runtime");
            }
            sandbox->qjsContext = JS_NewContext(sandbox->qjsRuntime);
            if (!sandbox->qjsContext) {
                JS_FreeRuntime(sandbox->qjsRuntime);
                throw facebook::jsi::JSError(rt, "Failed to create QuickJS context");
            }

            JS_SetContextOpaque(sandbox->qjsContext, sandbox.get());

            // Inject native bridge caller inside QuickJS
            JSValue globalObj = JS_GetGlobalObject(sandbox->qjsContext);
            JS_SetPropertyStr(sandbox->qjsContext, globalObj, "__nativeCall",
                              JS_NewCFunction(sandbox->qjsContext, js_native_call, "__nativeCall", 2));
            JS_FreeValue(sandbox->qjsContext, globalObj);

            activeContexts[contextId] = std::move(sandbox);
            return facebook::jsi::Value(true);
        }
    ));

    // disposeContext(contextId: string): void
    sandboxObj.setProperty(rt, "disposeContext", facebook::jsi::Function::createFromHostFunction(
        rt,
        facebook::jsi::PropNameID::forAscii(rt, "disposeContext"),
        1,
        [](facebook::jsi::Runtime &rt, const facebook::jsi::Value &thisVal, const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value {
            if (count < 1 || !args[0].isString()) {
                throw facebook::jsi::JSError(rt, "disposeContext requires a string contextId");
            }
            std::string contextId = args[0].getString(rt).utf8(rt);

            std::lock_guard<std::mutex> lock(activeContextsMutex);
            activeContexts.erase(contextId); // Destructor automatically frees QJS context and runtime
            return facebook::jsi::Value::undefined();
        }
    ));

    // registerHostCallback(contextId: string, callback: (contextId: string, action: string, argsJson: string) => string): void
    sandboxObj.setProperty(rt, "registerHostCallback", facebook::jsi::Function::createFromHostFunction(
        rt,
        facebook::jsi::PropNameID::forAscii(rt, "registerHostCallback"),
        2,
        [](facebook::jsi::Runtime &rt, const facebook::jsi::Value &thisVal, const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value {
            if (count < 2 || !args[0].isString() || !args[1].isObject() || !args[1].getObject(rt).isFunction(rt)) {
                throw facebook::jsi::JSError(rt, "registerHostCallback requires contextId (string) and callback (function)");
            }
            std::string contextId = args[0].getString(rt).utf8(rt);
            auto callback = args[1].getObject(rt).getFunction(rt);

            std::lock_guard<std::mutex> lock(activeContextsMutex);
            auto it = activeContexts.find(contextId);
            if (it == activeContexts.end()) {
                throw facebook::jsi::JSError(rt, "Context not found: " + contextId);
            }

            it->second->hermesRuntime = &rt;
            it->second->hermesCallback = std::make_shared<facebook::jsi::Function>(std::move(callback));
            return facebook::jsi::Value::undefined();
        }
    ));

    // evalCode(contextId: string, code: string, filename: string): string
    sandboxObj.setProperty(rt, "evalCode", facebook::jsi::Function::createFromHostFunction(
        rt,
        facebook::jsi::PropNameID::forAscii(rt, "evalCode"),
        3,
        [](facebook::jsi::Runtime &rt, const facebook::jsi::Value &thisVal, const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value {
            if (count < 3 || !args[0].isString() || !args[1].isString() || !args[2].isString()) {
                throw facebook::jsi::JSError(rt, "evalCode requires contextId (string), code (string), and filename (string)");
            }
            std::string contextId = args[0].getString(rt).utf8(rt);
            std::string code = args[1].getString(rt).utf8(rt);
            std::string filename = args[2].getString(rt).utf8(rt);

            SandboxContext *sandbox = nullptr;
            {
                std::lock_guard<std::mutex> lock(activeContextsMutex);
                auto it = activeContexts.find(contextId);
                if (it == activeContexts.end()) {
                    throw facebook::jsi::JSError(rt, "Context not found: " + contextId);
                }
                sandbox = it->second.get();
            }

            // Temporarily update Hermes runtime in case it changed (reloads)
            sandbox->hermesRuntime = &rt;

            JSValue evalResult = JS_Eval(sandbox->qjsContext, code.c_str(), code.length(), filename.c_str(), JS_EVAL_TYPE_GLOBAL);

            if (JS_IsException(evalResult)) {
                JSValue exception = JS_GetException(sandbox->qjsContext);
                const char *msg = JS_ToCString(sandbox->qjsContext, exception);
                std::string errorStr = msg ? msg : "Unknown QuickJS exception";
                if (msg) JS_FreeCString(sandbox->qjsContext, msg);

                JSValue stack = JS_GetPropertyStr(sandbox->qjsContext, exception, "stack");
                if (!JS_IsUndefined(stack)) {
                    const char *stackMsg = JS_ToCString(sandbox->qjsContext, stack);
                    if (stackMsg) {
                        errorStr += "\n" + std::string(stackMsg);
                        JS_FreeCString(sandbox->qjsContext, stackMsg);
                    }
                }
                JS_FreeValue(sandbox->qjsContext, stack);
                JS_FreeValue(sandbox->qjsContext, exception);
                JS_FreeValue(sandbox->qjsContext, evalResult);

                throw facebook::jsi::JSError(rt, errorStr);
            }

            if (JS_IsUndefined(evalResult)) {
                JS_FreeValue(sandbox->qjsContext, evalResult);
                return facebook::jsi::Value::undefined();
            }

            const char *resStr = JS_ToCString(sandbox->qjsContext, evalResult);
            std::string result(resStr ? resStr : "");
            if (resStr) JS_FreeCString(sandbox->qjsContext, resStr);
            JS_FreeValue(sandbox->qjsContext, evalResult);

            return facebook::jsi::String::createFromUtf8(rt, result);
        }
    ));

    // executePendingJobs(contextId: string): boolean
    sandboxObj.setProperty(rt, "executePendingJobs", facebook::jsi::Function::createFromHostFunction(
        rt,
        facebook::jsi::PropNameID::forAscii(rt, "executePendingJobs"),
        1,
        [](facebook::jsi::Runtime &rt, const facebook::jsi::Value &thisVal, const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value {
            if (count < 1 || !args[0].isString()) {
                throw facebook::jsi::JSError(rt, "executePendingJobs requires contextId (string)");
            }
            std::string contextId = args[0].getString(rt).utf8(rt);

            SandboxContext *sandbox = nullptr;
            {
                std::lock_guard<std::mutex> lock(activeContextsMutex);
                auto it = activeContexts.find(contextId);
                if (it == activeContexts.end()) {
                    return facebook::jsi::Value(false);
                }
                sandbox = it->second.get();
            }

            JSContext *ctx1 = nullptr;
            int runCount = 0;
            while (JS_ExecutePendingJob(sandbox->qjsRuntime, &ctx1) > 0) {
                runCount++;
            }
            return facebook::jsi::Value(runCount > 0);
        }
    ));

    rt.global().setProperty(rt, "QuickJSSandbox", sandboxObj);
}

} // namespace quickjssandbox

"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.QuickjsSandbox = void 0;
const react_native_1 = require("react-native");
// Force load the native module to trigger initialization and install JSI bindings
const QuickjsSandboxModule = react_native_1.NativeModules.QuickjsSandbox ?? react_native_1.TurboModuleRegistry.get('QuickjsSandbox');
if (QuickjsSandboxModule && typeof QuickjsSandboxModule.multiply === 'function') {
    QuickjsSandboxModule.multiply(1, 1);
}
function getSandbox() {
    const sandbox = globalThis.QuickJSSandbox;
    if (!sandbox) {
        const globalProcess = globalThis.process;
        if (globalProcess && globalProcess.env && globalProcess.env.NODE_ENV === 'test') {
            return {
                createContext: () => true,
                disposeContext: () => { },
                registerHostCallback: () => { },
                evalCode: () => '',
                executePendingJobs: () => true,
            };
        }
        throw new Error('QuickJSSandbox JSI bindings are not installed. Make sure the native module is built and loaded.');
    }
    return sandbox;
}
exports.QuickjsSandbox = {
    createContext(contextId) {
        return getSandbox().createContext(contextId);
    },
    disposeContext(contextId) {
        getSandbox().disposeContext(contextId);
    },
    registerHostCallback(contextId, callback) {
        getSandbox().registerHostCallback(contextId, (_ctxId, action, argsJson) => {
            return callback(action, argsJson);
        });
    },
    evalCode(contextId, code, filename) {
        return getSandbox().evalCode(contextId, code, filename);
    },
    executePendingJobs(contextId) {
        return getSandbox().executePendingJobs(contextId);
    },
};
exports.default = exports.QuickjsSandbox;

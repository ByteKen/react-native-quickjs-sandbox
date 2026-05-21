import { NativeModules, TurboModuleRegistry } from 'react-native';

// Force load the native module to trigger initialization and install JSI bindings
const QuickjsSandboxModule = NativeModules.QuickjsSandbox ?? TurboModuleRegistry.get('QuickjsSandbox');
if (QuickjsSandboxModule && typeof QuickjsSandboxModule.multiply === 'function') {
  QuickjsSandboxModule.multiply(1, 1);
}

export interface JSIQuickJSSandbox {
  createContext(contextId: string): boolean;
  disposeContext(contextId: string): void;
  registerHostCallback(
    contextId: string,
    callback: (contextId: string, action: string, argsJson: string) => string | undefined
  ): void;
  evalCode(contextId: string, code: string, filename: string): string;
  executePendingJobs(contextId: string): boolean;
}

declare global {
  var QuickJSSandbox: JSIQuickJSSandbox | undefined;
}

function getSandbox(): JSIQuickJSSandbox {
  const sandbox = (globalThis as any).QuickJSSandbox;
  if (!sandbox) {
    const globalProcess = (globalThis as any).process;
    if (globalProcess && globalProcess.env && globalProcess.env.NODE_ENV === 'test') {
      return {
        createContext: () => true,
        disposeContext: () => {},
        registerHostCallback: () => {},
        evalCode: () => '',
        executePendingJobs: () => true,
      };
    }
    throw new Error(
      'QuickJSSandbox JSI bindings are not installed. Make sure the native module is built and loaded.'
    );
  }
  return sandbox;
}

export const QuickjsSandbox = {
  createContext(contextId: string): boolean {
    return getSandbox().createContext(contextId);
  },

  disposeContext(contextId: string): void {
    getSandbox().disposeContext(contextId);
  },

  registerHostCallback(
    contextId: string,
    callback: (action: string, argsJson: string) => string | undefined
  ): void {
    getSandbox().registerHostCallback(contextId, (_ctxId, action, argsJson) => {
      return callback(action, argsJson);
    });
  },

  evalCode(contextId: string, code: string, filename: string): string {
    return getSandbox().evalCode(contextId, code, filename);
  },

  executePendingJobs(contextId: string): boolean {
    return getSandbox().executePendingJobs(contextId);
  },
};
export default QuickjsSandbox;

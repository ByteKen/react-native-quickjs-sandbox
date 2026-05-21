# react-native-quickjs-sandbox

A high-performance React Native JavaScript sandboxing library powered by the QuickJS engine and React Native JSI. Run untrusted scripts synchronously in isolated execution contexts.

> [!WARNING]
> **Notice: This package has not been fully tested or verified on iOS yet.** 
> It is currently confirmed and stable on Android. iOS support is present in the build configurations (CMake/Podspec) but is untested on physical devices or simulator setups. Use with caution on iOS.

---

## Features

* **Synchronous Execution (JSI)**: Invokes sandbox operations synchronously with no serialization overhead.
* **Isolated Contexts**: Create multiple independent execution sandboxes with isolated global variables.
* **Host Callback Support**: Expose native or React Native functions synchronously to sandboxed scripts via a global `__hostCall` helper.
* **Lifecycle Controls**: Clean up native engine memory allocations immediately.

---

## Installation

```bash
npm install react-native-quickjs-sandbox
```

---

## Usage

```typescript
import { QuickjsSandbox } from 'react-native-quickjs-sandbox';

const CONTEXT_ID = 'my-sandbox-1';

// 1. Initialize a context
QuickjsSandbox.createContext(CONTEXT_ID);

// 2. Evaluate code (returns result as a string)
try {
  const result = QuickjsSandbox.evalCode(CONTEXT_ID, 'const a = 10; const b = 20; a + b;', 'main.js');
  console.log('Result:', result); // "30"
} catch (error) {
  console.error('Execution error:', error);
}

// 3. Register a synchronous host callback
QuickjsSandbox.registerHostCallback(CONTEXT_ID, (action: string, argsJson: string): string | undefined => {
  const args = JSON.parse(argsJson);
  if (action === 'log') {
    console.log('[Sandbox Log]:', args.message);
    return 'success';
  }
  return undefined;
});

// Run a script calling the host callback
const script = `__hostCall('log', JSON.stringify({ message: 'Hello from sandbox!' }));`;
QuickjsSandbox.evalCode(CONTEXT_ID, script, 'interop.js');

// 4. Dispose of context to release memory
QuickjsSandbox.disposeContext(CONTEXT_ID);
```

---

## Platform Details

* **Android**: Uses CMake to build and compile the QuickJS C engine directly.
* **iOS**: CocoaPods build configurations download the QuickJS source files dynamically during `pod install`. *(Untested on live environments)*.

---

## License

MIT

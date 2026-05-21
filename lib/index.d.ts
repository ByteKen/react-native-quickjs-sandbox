export interface JSIQuickJSSandbox {
    createContext(contextId: string): boolean;
    disposeContext(contextId: string): void;
    registerHostCallback(contextId: string, callback: (contextId: string, action: string, argsJson: string) => string | undefined): void;
    evalCode(contextId: string, code: string, filename: string): string;
    executePendingJobs(contextId: string): boolean;
}
declare global {
    var QuickJSSandbox: JSIQuickJSSandbox | undefined;
}
export declare const QuickjsSandbox: {
    createContext(contextId: string): boolean;
    disposeContext(contextId: string): void;
    registerHostCallback(contextId: string, callback: (action: string, argsJson: string) => string | undefined): void;
    evalCode(contextId: string, code: string, filename: string): string;
    executePendingJobs(contextId: string): boolean;
};
export default QuickjsSandbox;

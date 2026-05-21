"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.multiply = multiply;
const NativeQuickjsSandbox_1 = __importDefault(require("./NativeQuickjsSandbox"));
function multiply(a, b) {
    return NativeQuickjsSandbox_1.default.multiply(a, b);
}

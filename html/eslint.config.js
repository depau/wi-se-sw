import js from "@eslint/js";
import tseslint from "typescript-eslint";
// import pluginPreact from "eslint-plugin-preact";

/**
 * ESLint 9.x + @typescript-eslint 8.x
 */
export default [
  // Basic JS ruleset
  js.configs.recommended,

  // Basic TypeScript ruleset
  ...tseslint.configs.recommended.map(config => ({
    ...config,
    languageOptions: {
      ...config.languageOptions,
      parser: tseslint.parser
    }
  })),

  {
    files: ["src/**/*.{ts,tsx,js,jsx}"],
    ignores: ["dist/**"],
    languageOptions: {
      parser: tseslint.parser,
      parserOptions: {
        ecmaVersion: "latest",
        sourceType: "module",
        ecmaFeatures: { jsx: true }
      }
    },
    plugins: {
      // preact: pluginPreact
    },
    rules: {
      // Common
      "no-console": devMode() ? "off" : ["warn", { allow: ["warn", "error"] }],
      "no-unused-vars": "off", // or warn
      "@typescript-eslint/no-unused-vars": ["warn", { argsIgnorePattern: "^_", varsIgnorePattern: "^h$" }],
      '@typescript-eslint/no-explicit-any': 'off',

      // Preact
      // "preact/jsx-no-duplicate-props": "warn",
      // "preact/jsx-no-undef": "error",
      // "preact/no-unknown-property": "warn"
      // "preact/jsx-boolean-value": "warn",
      // "preact/jsx-no-duplicate-props": "error",
      // "preact/jsx-no-script-url": "error",
      // "preact/jsx-no-target-blank": "error",
    }
  }
];

/**
 * Helper to detect dev/prod mode
 */
function devMode() {
  return process.env.NODE_ENV !== "production";
}

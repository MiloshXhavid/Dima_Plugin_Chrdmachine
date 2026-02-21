# Coding Conventions

**Analysis Date:** 2026-02-21

## Naming Patterns

**Files:**
- Source files: `*.cjs` (CommonJS modules)
- Test files: `*.test.cjs` (co-located with test execution)
- Example: `state.cjs`, `state.test.cjs`

**Functions:**
- PascalCase for command handlers: `cmdStateLoad`, `cmdStateGet`, `cmdStatePatch`
- camelCase for internal helpers: `safeReadFile`, `loadConfig`, `normalizePhaseName`
- Prefix with `cmd` for CLI command functions

**Variables:**
- camelCase: `tmpDir`, `result`, `content`, `stateRaw`
- Snake_case for object keys (API output): `current_phase`, `state_exists`, `roadmap_exists`
- Const declarations for all module-level values

**Types/Objects:**
- UPPER_SNAKE_CASE for constants: `MODEL_PROFILES`, `DEFAULTS`
- PascalCase for class-like objects if used

## Code Style

**Formatting:**
- No automatic formatter detected (no Prettier, no ESLint config)
- Manual indentation: 2 spaces
- No trailing commas
- Single quotes for strings

**Linting:**
- Not configured - no ESLint or similar
- Manual code review required

**General Style:**
- Functions are generally small and focused (20-80 lines typical)
- Early returns for error conditions
- One variable declaration per line preferred

## Import Organization

**Order:**
1. Node.js built-ins: `require('fs')`, `require('path')`, `require('child_process')`
2. Local project imports: `require('./core.cjs')`, `require('./helpers.cjs')`

**Path Patterns:**
- Relative imports with `./` prefix for local modules
- No path aliases configured

**Example:**
```javascript
const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');
const { loadConfig, output, error } = require('./core.cjs');
```

## Error Handling

**Patterns:**
- Try-catch with empty catch block for optional file reads:
  ```javascript
  try {
    stateRaw = fs.readFileSync(path.join(planningDir, 'STATE.md'), 'utf-8');
  } catch {}
  ```
- Explicit error function calls for CLI errors:
  ```javascript
  error('STATE.md not found');
  ```
- Result objects for graceful degradation:
  ```javascript
  const result = { success: true, output: result.trim() };
  // vs
  return { success: false, error: err.message };
  ```

**Output:**
- Use `output()` helper for JSON responses
- Use `error()` helper for error messages (writes to stderr, exits with code 1)
- Success exits with code 0

## Logging

**Framework:** None (uses console implicitly via output helpers)

**Patterns:**
- No explicit logging statements in source code
- Output via `output()` function handles JSON serialization
- Error messages via `error()` helper

## Comments

**When to Comment:**
- JSDoc at file level: `/**\n * Core — Shared utilities...\n */`
- Function purpose comments: `// ─── Output helpers ────────────────────────────────────────────────────`
- Section dividers for logical groupings

**Style:**
- Double asterisk for file-level JSDoc: `/** ... */`
- Single line comments for section headers: `// ─── Name ───`
- No inline comments except for complex regex patterns

## Function Design

**Size:** Small, focused functions (often < 50 lines)

**Parameters:**
- Named parameters via destructuring for options objects
- Multiple optional parameters handled via options pattern
- Example: `cmdStateRecordMetric(cwd, options, raw)` where options contains `{ phase, plan, duration, tasks, files }`

**Return Values:**
- Return early pattern for errors
- Return result objects for complex operations
- No return for void operations (state modifications)

## Module Design

**Exports:**
- CommonJS module.exports at end of file:
  ```javascript
  module.exports = {
    functionName,
    anotherFunction,
  };
  ```

**Barrel Files:**
- Not used - direct imports from individual modules

---

*Convention analysis: 2026-02-21*

# Testing Patterns

**Analysis Date:** 2026-02-21

## Test Framework

**Runner:**
- Node.js built-in test runner: `node --test`
- Version: Node.js 16.7.0+ (from package.json engines)
- Test files: `tests/*.test.cjs`

**Assertion Library:**
- Node.js built-in `node:assert` module
- Common assertions: `strictEqual`, `deepStrictEqual`, `ok`

**Run Commands:**
```bash
npm test                    # Run all tests (node --test tests/*.test.cjs)
```

## Test File Organization

**Location:**
- All tests in `tests/` directory (separate from source)
- Test helper: `tests/helpers.cjs`

**Naming:**
- Pattern: `{module}.test.cjs` (e.g., `state.test.cjs`, `phase.test.cjs`)
- Helper file: `helpers.cjs` (not a test file)

**Structure:**
```
tests/
├── helpers.cjs         # Shared test utilities
├── commands.test.cjs
├── init.test.cjs
├── milestone.test.cjs
├── phase.test.cjs
├── roadmap.test.cjs
├── state.test.cjs
└── verify.test.cjs
```

## Test Structure

**Suite Organization:**
```javascript
const { test, describe, beforeEach, afterEach } = require('node:test');
const assert = require('node:assert');
const fs = require('fs');
const path = require('path');
const { runGsdTools, createTempProject, cleanup } = require('./helpers.cjs');

describe('command-name', () => {
  let tmpDir;

  beforeEach(() => {
    tmpDir = createTempProject();
  });

  afterEach(() => {
    cleanup(tmpDir);
  });

  test('test description', () => {
    // Arrange: create test files/directories
    // Act: run command
    // Assert: check results
  });
});
```

**Patterns:**
- Setup: `createTempProject()` creates temp directory with `.planning/phases` structure
- Teardown: `cleanup(tmpDir)` removes temp directory
- Each test is independent and self-contained
- Tests use actual CLI commands via `runGsdTools()` helper

## Mocking

**Framework:** Not used

**Patterns:**
- No mocking framework (sinon, jest.mock, etc.)
- Tests use real filesystem operations with temp directories
- Tests invoke actual CLI commands end-to-end
- No unit-level mocking of internal functions

**What to Mock:**
- Nothing - tests are integration-style

**What NOT to Mock:**
- File system operations
- CLI commands themselves

## Fixtures and Factories

**Test Data:**
- Created inline in each test via `fs.writeFileSync()`
- Helper function `createTempProject()` creates basic directory structure

**Location:**
- Inline fixtures within test files
- Common helpers in `tests/helpers.cjs`

**Example Pattern:**
```javascript
test('extracts basic fields from STATE.md', () => {
  fs.writeFileSync(
    path.join(tmpDir, '.planning', 'STATE.md'),
    `# Project State

**Current Phase:** 03
**Current Phase Name:** API Layer
...
`
  );

  const result = runGsdTools('state-snapshot', tmpDir);
  assert.ok(result.success, `Command failed: ${result.error}`);

  const output = JSON.parse(result.output);
  assert.strictEqual(output.current_phase, '03', 'current phase extracted');
});
```

## Coverage

**Requirements:** None enforced

**View Coverage:** Not available (no coverage tool configured)

## Test Types

**Unit Tests:**
- Not used - tests are integration-style
- Commands tested end-to-end via CLI

**Integration Tests:**
- Primary testing approach
- Tests run actual CLI commands (`node gsd-tools.cjs <command>`)
- Tests create temp directories, write files, execute commands, verify outputs
- JSON output parsed and asserted

**E2E Tests:**
- Not used

## Common Patterns

**Async Testing:**
- Synchronous tests only (Node.js test runner supports async but not used)
- Commands execute synchronously via `execSync`

**Error Testing:**
```javascript
test('missing STATE.md returns error', () => {
  const result = runGsdTools('state-snapshot', tmpDir);
  assert.ok(result.success, `Command should succeed: ${result.error}`);

  const output = JSON.parse(result.output);
  assert.strictEqual(output.error, 'STATE.md not found', 'should report missing file');
});
```

**CLI Output Parsing:**
- All commands output JSON
- Tests parse with `JSON.parse(result.output)`
- Check `result.success` boolean for command success/failure

**Test Helpers (from `tests/helpers.cjs`):**
```javascript
function runGsdTools(args, cwd = process.cwd()) {
  try {
    const result = execSync(`node "${TOOLS_PATH}" ${args}`, {
      cwd,
      encoding: 'utf-8',
      stdio: ['pipe', 'pipe', 'pipe'],
    });
    return { success: true, output: result.trim() };
  } catch (err) {
    return {
      success: false,
      output: err.stdout?.toString().trim() || '',
      error: err.stderr?.toString().trim() || err.message,
    };
  }
}

function createTempProject() {
  const tmpDir = fs.mkdtempSync(path.join(require('os').tmpdir(), 'gsd-test-'));
  fs.mkdirSync(path.join(tmpDir, '.planning', 'phases'), { recursive: true });
  return tmpDir;
}

function cleanup(tmpDir) {
  fs.rmSync(tmpDir, { recursive: true, force: true });
}
```

**Section Dividers:**
- Tests use ASCII dividers to separate command groups:
  ```javascript
  // ─────────────────────────────────────────────────────────────────────────────
  // command-name command
  // ─────────────────────────────────────────────────────────────────────────────
  ```

---

*Testing analysis: 2026-02-21*

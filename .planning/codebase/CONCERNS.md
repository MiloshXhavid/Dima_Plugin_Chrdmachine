# Codebase Concerns

**Analysis Date:** 2026-02-21

## Tech Debt

**Monolithic install.js:**
- Issue: `bin/install.js` is ~1900 lines of single-file complexity
- Files: `bin/install.js`
- Impact: Hard to maintain, test, and understand. Any change risks breaking multiple runtime installations
- Fix approach: Split into smaller modules (config-handler.js, runtime-installers.js, etc.)

**Custom JSONC parser:**
- Issue: Hand-rolled `parseJsonc()` function (lines 1056-1110 in install.js) handles comments/trailing commas manually
- Files: `bin/install.js`
- Impact: May miss edge cases in complex JSONC files. Maintenance burden
- Fix approach: Add jsonc-parser dependency or use a proven library

**Plain JavaScript without TypeScript:**
- Issue: No type checking - all runtime types are implicit
- Files: All `.js` files (`bin/install.js`, `hooks/*.js`, `scripts/*.js`)
- Impact: Runtime errors instead of compile-time detection. Harder to refactor safely
- Fix approach: Add TypeScript with incremental migration

**Complex path replacement logic:**
- Issue: Multiple regex-based path replacements for runtime-specific paths (`~/.claude/`, `./.claude/`, etc.)
- Files: `bin/install.js` (functions: copyWithPathReplacement, copyFlattenedCommands)
- Impact: Fragile - any format change in source files could break installation
- Fix approach: Use a template engine or structured replacement

## Known Bugs

**Attribution cache behavior:**
- Issue: Attribution cache is populated once at install time and never refreshed during runtime
- Files: `bin/install.js` (lines 219-263)
- Trigger: User changes attribution setting after GSD is installed
- Workaround: Reinstall GSD to pick up new settings

**Hook path construction on Windows:**
- Issue: Uses forward slashes for cross-platform compatibility, but Windows cmd.exe may have issues with quoted paths containing forward slashes
- Files: `bin/install.js` (buildHookCommand function)
- Impact: Unclear if hooks execute correctly on all Windows configurations
- Workaround: Not explicitly tested

## Security Considerations

**execSync in test helpers:**
- Risk: Test helper uses execSync with dynamic command construction
- Files: `tests/helpers.cjs` (line 14)
- Current mitigation: Commands are static strings, not user-controlled
- Recommendations: Consider using child_process spawn with sanitized inputs

**npm view network call on every session:**
- Risk: `gsd-check-update.js` runs `npm view get-shit-done-cc version` on every Claude session start
- Files: `hooks/gsd-check-update.js`
- Current mitigation: Results are cached, but still makes network request
- Recommendations: Reduce check frequency (once per day, not per session)

**File system access in hooks:**
- Risk: Multiple hooks read/write to tmp directory and config directories
- Files: `hooks/gsd-statusline.js`, `hooks/gsd-context-monitor.js`, `hooks/gsd-check-update.js`
- Current mitigation: Silent fail on errors - hooks never break tool execution
- Recommendations: Document security model for users

## Performance Bottlenecks

**Context monitor file I/O:**
- Problem: Writes bridge file on every statusline render, reads on every tool use
- Files: `hooks/gsd-statusline.js` (writes to /tmp/claude-ctx-*.json), `hooks/gsd-context-monitor.js` (reads from same)
- Cause: IPC via filesystem instead of memory
- Improvement path: Use process environment or stdin/stdout for passing context

**npm view timeout on network failure:**
- Problem: 10-second timeout on npm view can block session startup
- Files: `hooks/gsd-check-update.js` (line 45)
- Cause: Network call with timeout runs in background but still has overhead
- Improvement path: Use cached result with TTL instead of always checking

## Fragile Areas

**Install.js path replacement:**
- Files: `bin/install.js` (copyWithPathReplacement, copyFlattenedCommands)
- Why fragile: Uses regex patterns like `/~\/\.claude\//g` - any format change in source markdown breaks installation
- Safe modification: Add comprehensive test cases for path replacement before any changes
- Test coverage: No unit tests for install logic

**Frontmatter conversion logic:**
- Files: `bin/install.js` (convertClaudeToOpencodeFrontmatter, convertClaudeToGeminiAgent)
- Why fragile: Complex regex and state machine for YAML parsing - edge cases likely
- Safe modification: Add edge case tests before modifying conversion logic
- Test coverage: No tests for conversion functions

**Runtime detection and directory resolution:**
- Files: `bin/install.js` (getGlobalDir, getOpencodeGlobalDir)
- Why fragile: Multiple environment variables and fallbacks make behavior hard to predict
- Safe modification: Document all configuration options clearly
- Test coverage: No tests for directory resolution

## Scaling Limits

**Hooks directory:**
- Current capacity: Assumes single GSD installation per runtime
- Limit: No support for multiple concurrent GSD versions
- Scaling path: Add version-aware hook naming

**Memory-based context bridge:**
- Current capacity: Single session context
- Limit: Doesn't work across Claude Code restarts
- Scaling path: Persist to disk instead of tmp

## Dependencies at Risk

**esbuild (devDependency):**
- Risk: Security vulnerabilities in bundler dependencies
- Impact: Build process could be compromised
- Migration plan: Already minimal - could remove if hooks are not bundled

**npm CLI dependency:**
- Risk: npm command must be available in PATH for update checking
- Impact: Update checks fail on systems without npm
- Migration plan: Use npm registry API instead of CLI

## Missing Critical Features

**TypeScript support:**
- Problem: All source code is plain JavaScript
- Blocks: Safe refactoring, IDE autocompletion, compile-time error detection

**Install logic unit tests:**
- Problem: No tests for the complex install.js functions
- Blocks: Safe evolution of installation code

**Hook integration tests:**
- Problem: No end-to-end tests verifying hooks work with Claude Code
- Blocks: Confidence that hooks work in production

## Test Coverage Gaps

**Untested: Install/uninstall logic:**
- What's not tested: Full install flow, uninstall flow, path replacement, runtime conversion
- Files: `bin/install.js`
- Risk: Bugs in installation could corrupt user configurations unnoticed
- Priority: High

**Untested: Hook execution:**
- What's not tested: Hooks actually work with Claude Code
- Files: `hooks/gsd-*.js`
- Risk: Hooks might fail silently in production
- Priority: High

**Untested: Runtime conversion:**
- What's not tested: Claude → OpenCode, Claude → Gemini conversions
- Files: `bin/install.js` (conversion functions)
- Risk: Commands/workflows may not work on OpenCode or Gemini
- Priority: Medium

---

*Concerns audit: 2026-02-21*

# Technology Stack

**Analysis Date:** 2026-02-21

## Languages

**Primary:**
- JavaScript (Node.js) - All source code is pure JavaScript
- Shell scripts - `.sh` hooks for Unix compatibility

**Not Used:**
- TypeScript - No TypeScript files in the source codebase

## Runtime

**Environment:**
- Node.js >= 16.7.0
- Platforms: macOS, Windows, Linux (cross-platform)

**Package Manager:**
- npm (Node Package Manager)
- Lockfile: `package-lock.json` present

## Frameworks

**Core:**
- None - This is a vanilla Node.js CLI application
- No web frameworks (Express, Fastify, etc.)
- No UI frameworks

**Build/Dev:**
- esbuild ^0.24.0 - Used for building hooks (dev dependency only)
- Scripts located in `scripts/build-hooks.js`

**Testing:**
- Node.js built-in test runner
- Command: `node --test tests/*.test.cjs`
- Test files in CommonJS format (`.test.cjs`)

## Key Dependencies

**Runtime:**
- None - Zero runtime dependencies in `package.json`

**DevDependencies:**
- esbuild ^0.24.0 - Build tool for hooks

**Node.js Built-in Modules Used:**
- `fs` - File system operations
- `path` - Path manipulation
- `os` - OS utilities (homedir, tmpdir)
- `crypto` - Hashing (SHA256 for file manifests)
- `readline` - Interactive CLI prompts
- `child_process` - Spawning background processes
- `JSON` - JSON parsing and serialization

## Configuration

**Project Configuration:**
- `package.json` - NPM package definition
- `package-lock.json` - Dependency lock file
- `.gitignore` - Git ignore rules

**No Configuration Files Found:**
- No `.eslintrc` / `eslint.config.*` - No linting
- No `.prettierrc` - No code formatting
- No `tsconfig.json` - No TypeScript
- No `jest.config` / `vitest.config` - Uses Node.js built-in test runner
- No `.nvmrc` - No Node version manager config

**Environment Variables:**
- Uses runtime environment variables for AI runtime configs:
  - `CLAUDE_CONFIG_DIR` - Claude Code config directory
  - `GEMINI_CONFIG_DIR` - Gemini CLI config directory
  - `OPENCODE_CONFIG_DIR` / `OPENCODE_CONFIG` - OpenCode config
  - `XDG_CONFIG_HOME` - For OpenCode XDG compliance
- No `.env` files - These are handled by the host AI runtimes

## Platform Requirements

**Development:**
- Node.js >= 16.7.0
- npm for package management

**Production:**
- Node.js >= 16.7.0 (runs as CLI tool)
- Installed to AI runtime config directories:
  - Claude Code: `~/.claude/` (or custom via `CLAUDE_CONFIG_DIR`)
  - OpenCode: `~/.config/opencode/` (or custom via `OPENCODE_CONFIG_DIR`)
  - Gemini CLI: `~/.gemini/` (or custom via `GEMINI_CONFIG_DIR`)

---

*Stack analysis: 2026-02-21*

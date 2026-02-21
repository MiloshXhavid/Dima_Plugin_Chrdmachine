# External Integrations

**Analysis Date:** 2026-02-21

## APIs & External Services

**AI Runtimes (Primary Integration):**

This is a meta-prompting system that integrates with three AI coding assistants:

- **Claude Code** (Anthropic)
  - Role: Primary supported runtime
  - Integration: Commands, agents, hooks, statusline
  - Config: `~/.claude/` or `CLAUDE_CONFIG_DIR` env var

- **OpenCode**
  - Role: Supported runtime
  - Integration: Flattened command structure, permission configuration
  - Config: `~/.config/opencode/` or `OPENCODE_CONFIG_DIR` env var

- **Gemini CLI** (Google)
  - Role: Supported runtime
  - Integration: Commands, agents (converted to TOML format)
  - Config: `~/.gemini/` or `GEMINI_CONFIG_DIR` env var

**NPM Registry:**
- Package: `get-shit-done-cc`
- Usage: Publishing and version checking
- Endpoint: `npm view get-shit-done-cc version` (called by `hooks/gsd-check-update.js`)

## Data Storage

**File-Based Storage (Primary):**
- No databases used
- All data stored in markdown files and JSON files in the AI runtime config directories

**State Files:**
- Project state: `.planning/` directory in user's project
- GSD metadata: `get-shit-done/` subdirectory in runtime config
- Cache: `~/.claude/cache/gsd-update-check.json`
- Todo files: `~/.claude/todos/` (Claude Code specific)

**Manifest Tracking:**
- File hash manifests for local patch detection: `gsd-file-manifest.json`
- Stored in runtime config directory root

## Authentication & Identity

**No External Auth Providers:**
- GSD does not implement any authentication
- Relies on the host AI runtime's authentication (Claude Code, OpenCode, Gemini CLI)

**Git Commit Attribution:**
- Parses runtime settings for commit attribution config
- Claude Code: `settings.json` → `attribution.commit`
- OpenCode: `opencode.json` → `disable_ai_attribution`
- Gemini: `settings.json` → `attribution.commit`

## Monitoring & Observability

**Statusline Hook:**
- File: `hooks/gsd-statusline.js`
- Displays: Model name, current task, directory, context usage percentage
- Uses Claude Code's statusline API via stdin/stdout JSON

**Context Monitor Hook:**
- File: `hooks/gsd-context-monitor.js`
- Monitors: Context window usage
- Injects warnings when context drops below thresholds (35% = warning, 25% = critical)

**Update Checker:**
- File: `hooks/gsd-check-update.js`
- Checks: NPM registry for new versions
- Runs: Once per session via SessionStart hook
- Caches: Results in `~/.claude/cache/gsd-update-check.json`

**No External Monitoring Services:**
- No Sentry, Datadog, or similar
- No external logging

## CI/CD & Deployment

**Hosting:**
- npm registry (public package)
- GitHub repository: `glittercowboy/get-shit-done`

**CI/CD:**
- GitHub Actions workflows present in `.github/` directory
- No specific CI service configured for this project

**Installation:**
- Distributed via `npx get-shit-done-cc@latest`
- Self-installing to AI runtime config directories

## Environment Configuration

**Runtime Config Locations (Auto-Detected):**

Claude Code:
```
--config-dir > CLAUDE_CONFIG_DIR > ~/.claude
```

OpenCode:
```
--config-dir > OPENCODE_CONFIG_DIR > OPENCODE_CONFIG dirname > XDG_CONFIG_HOME/opencode > ~/.config/opencode
```

Gemini CLI:
```
--config-dir > GEMINI_CONFIG_DIR > ~/.gemini
```

**No Required Environment Variables:**
- GSD reads host runtime configs, not env vars directly

## Webhooks & Callbacks

**Incoming:**
- None - GSD does not expose any HTTP endpoints
- All communication is via AI runtime hooks (stdin/stdout JSON)

**Outgoing:**
- None - GSD makes no outbound HTTP requests except:
  - `npm view get-shit-done-cc version` for update checking (optional, background)

## File-Based Protocol

**Installation Manifest:**
- Format: JSON
- Location: `{configDir}/gsd-file-manifest.json`
- Purpose: Track installed files for patch detection

**Local Patches:**
- Directory: `{configDir}/gsd-local-patches/`
- Purpose: Store user modifications to GSD files for reapplication after updates

**Context Bridge:**
- Location: `/tmp/claude-ctx-{session_id}.json`
- Purpose: Pass context metrics from statusline hook to context monitor hook

---

*Integration audit: 2026-02-21*

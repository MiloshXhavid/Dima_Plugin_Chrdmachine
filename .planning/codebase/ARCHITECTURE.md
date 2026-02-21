# Architecture

**Analysis Date:** 2026-02-21

## Pattern Overview

**Overall:** Multi-agent meta-prompting system for AI code assistants (Claude Code, OpenCode, Gemini CLI)

**Key Characteristics:**
- Plugin/extension architecture that installs into AI assistant config directories
- Agent-based execution using specialized sub-agents (planner, executor, verifier, debugger, etc.)
- Markdown + YAML frontmatter as the primary data structure and prompt template format
- Command-based orchestration through slash commands (`/gsd:*`)
- CLI tools as backend for complex operations (state management, phase initialization, etc.)

## Layers

**CLI/Installer Layer:**
- Purpose: Main entry point, handles installation/uninstallation across runtimes
- Location: `bin/install.js`
- Contains: Runtime detection, config directory resolution, file copying, permission configuration
- Depends on: Node.js fs/path modules
- Used by: `npx get-shit-done-cc` command

**Hook Layer:**
- Purpose: Runtime integration with Claude Code sessions
- Location: `hooks/`
- Contains: `gsd-statusline.js` (status display), `gsd-context-monitor.js` (context tracking), `gsd-check-update.js` (update notifications)
- Depends on: Claude Code hooks API
- Used by: Claude Code settings.json hook registrations

**Command Layer:**
- Purpose: User-invokable slash commands
- Location: `commands/gsd/`
- Contains: 32 command definitions as markdown files with YAML frontmatter
- Examples: `add-phase.md`, `execute-phase.md`, `plan-phase.md`, `map-codebase.md`
- Used by: Claude Code `/gsd:*` command invocation

**Agent Layer:**
- Purpose: Specialized AI sub-agents for different tasks
- Location: `agents/`
- Contains: 11 agent definitions (gsd-executor.md, gsd-verifier.md, gsd-planner.md, gsd-debugger.md, etc.)
- Each agent has: role definition, tools allowed, execution flow, success criteria
- Used by: Commands and workflows that spawn sub-agents

**Workflow/Template Layer:**
- Purpose: Reusable workflow definitions and templates
- Location: `get-shit-done/workflows/`, `get-shit-done/templates/`
- Contains: Phase workflows, verification patterns, templates for planning artifacts
- Examples: `execute-phase.md`, `plan-phase.md`, `verification-patterns.md`, `codebase/*.md`
- Used by: Agents and commands

**Core Library Layer:**
- Purpose: Backend logic for state management, phase operations, verification
- Location: `get-shit-done/bin/lib/*.cjs`
- Contains: `state.cjs`, `phase.cjs`, `milestone.cjs`, `roadmap.cjs`, `commands.cjs`, `init.cjs`, etc.
- Used by: Agents via Bash tool calls to `node ~/.claude/get-shit-done/bin/gsd-tools.cjs <command>`

**Reference Layer:**
- Purpose: Documentation and patterns for agents
- Location: `get-shit-done/references/`
- Contains: `git-integration.md`, `tdd.md`, `model-profiles.md`, `checkpoints.md`, etc.
- Used by: Agents via @-references in prompts

## Data Flow

**Installation Flow:**

1. User runs `npx get-shit-done-cc --claude --global`
2. `bin/install.js` detects runtime (Claude/OpenCode/Gemini)
3. Copies commands to `~/.claude/commands/gsd/` (or equivalent)
4. Copies get-shit-done skill to `~/.claude/get-shit-done/`
5. Copies agents to `~/.claude/agents/`
6. Installs hooks in `~/.claude/hooks/`
7. Updates `settings.json` with hook registrations
8. Writes manifest file for update detection

**Command Execution Flow:**

1. User invokes `/gsd:plan-phase`
2. Claude Code loads `commands/gsd/plan-phase.md`
3. Command prompt instructs Claude to run workflow
4. Claude spawns sub-agent via Task tool (e.g., `gsd-planner`)
5. Agent reads templates, executes, writes outputs
6. Agent calls CLI tools for state management
7. Results written to `.planning/` directory

**Agent Spawning Flow:**

1. Parent agent reads agent definition (e.g., `agents/gsd-executor.md`)
2. Extracts frontmatter: name, description, tools, color
3. Loads body as prompt template
4. Spawns Task with formatted prompt
5. Sub-agent executes with allowed tools
6. Returns results to parent

## Key Abstractions

**Command Definition:**
- Format: Markdown file with YAML frontmatter
- Frontmatter fields: name, description, tools (comma-separated), color
- Body: Prompt that Claude Code executes
- Example: `commands/gsd/execute-phase.md`

**Agent Definition:**
- Format: Markdown file with YAML frontmatter
- Frontmatter fields: name, description, tools, color
- Body sections: `<role>`, `<project_context>`, `<execution_flow>` with `<step>` tags
- Example: `agents/gsd-executor.md`

**State Management:**
- Location: `.planning/STATE.md`
- Format: Markdown with `**Field:** value` and `## Section` headers
- Managed by: `state.cjs` library
- Operations: load, get, patch, commit

**Phase Definition:**
- Location: `.planning/phases/<phase-name>/PHASE.md`
- Format: Markdown with YAML frontmatter + task list
- Frontmatter: phase, plan, type, autonomous, wave, depends_on, checkpoint
- Body: objective, context (@ references), tasks, verification criteria

**CLI Tool Interface:**
- Entry: `get-shit-done/bin/gsd-tools.cjs`
- Subcommands: init, state, phase, milestone, roadmap, verify, template
- Invocation: `node gsd-tools.cjs <subcommand> <args>`

## Entry Points

**Primary Entry:**
- Location: `bin/install.js`
- Triggers: `npx get-shit-done-cc [flags]`
- Responsibilities: Installation, uninstallation, runtime detection, file copying, config

**Slash Command Entry:**
- Location: `commands/gsd/*.md`
- Triggers: `/gsd:<command>` in Claude Code
- Responsibilities: Orchestrate workflows, delegate to agents

**Agent Entry:**
- Location: `agents/gsd-*.md`
- Triggers: Spawned via Task tool by commands or other agents
- Responsibilities: Execute specialized subtasks

**Hook Entry:**
- Location: `hooks/gsd-*.js`
- Triggers: Claude Code session events (SessionStart, PostToolUse, Stop)
- Responsibilities: Statusline display, context monitoring, update checking

**CLI Tool Entry:**
- Location: `get-shit-done/bin/gsd-tools.cjs`
- Triggers: Called via Bash tool by agents
- Responsibilities: State management, phase operations, verification

## Error Handling

**Strategy:** Best-effort with graceful degradation

**Patterns:**
- File system errors: Silent fail with comments (e.g., `// Silent fail - don't break statusline`)
- JSON parse errors: Return empty object, continue
- Missing files: Check existence before read, provide defaults
- Hook failures: Try-catch all, never throw to Claude Code

**Logging:**
- Console output via `console.log` with color codes
- Status messages: `${green}✓${reset}` success, `${yellow}⚠${reset}` warning
- Debug output via `${dim}` (ANSI dim)

## Cross-Cutting Concerns

**Path Resolution:**
- Global install: `~/.claude/`, `~/.config/opencode/`, `~/.gemini/`
- Local install: `./.claude/`, `./.opencode/`, `./.gemini/`
- Configurable via `--config-dir` flag or env vars (CLAUDE_CONFIG_DIR, etc.)

**Runtime Adaptation:**
- `bin/install.js` contains conversion functions for each runtime:
  - `convertClaudeToOpencodeFrontmatter()` - transforms YAML frontmatter
  - `convertClaudeToGeminiToml()` - converts to TOML format
  - Tool name mappings (Read → read_file for Gemini**Local)

 Patch Management:**
- Detects user modifications via SHA256 hash comparison
- Backs up to `gsd-local-patches/` directory
- Supports reapplication after updates

**Multi-Runtime Support:**
- Claude Code: commands/gsd/, agents/, hooks/, get-shit-done/
- OpenCode: command/ (flat), converts to .md with tools object
- Gemini: commands/gsd/, converts to .toml format

---

*Architecture analysis: 2026-02-21*

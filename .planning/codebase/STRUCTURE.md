# Codebase Structure

**Analysis Date:** 2026-02-21

## Directory Layout

```
get-shit-done/
├── bin/                    # Installation entry point
├── commands/gsd/          # Slash command definitions
├── get-shit-done/          # Main skill content
│   ├── workflows/          # Phase workflow definitions
│   ├── templates/          # Planning templates
│   ├── references/         # Agent reference docs
│   ├── bin/lib/            # Core JavaScript libraries
│   └── templates/codebase/ # Codebase analysis templates
├── agents/                 # Agent definitions
├── hooks/                  # Claude Code hooks
├── scripts/                # Build scripts
├── tests/                  # Test files
└── assets/                 # Logo assets
```

## Directory Purposes

**bin/:**
- Purpose: Primary entry point and installer
- Contains: `install.js` (main CLI tool)
- Key files: `install.js` - Handles installation for Claude Code, OpenCode, and Gemini

**commands/gsd/:**
- Purpose: Slash command definitions for Claude Code
- Contains: 32 command markdown files
- Key files: `add-phase.md`, `execute-phase.md`, `plan-phase.md`, `map-codebase.md`, `help.md`

**get-shit-done/:**
- Purpose: Main skill content installed to config directory
- Contains: Workflows, templates, references, and CLI tools
- This directory is copied as-is to `~/.claude/get-shit-done/`

**get-shit-done/workflows/:**
- Purpose: Phase execution workflows
- Contains: 27 workflow markdown files
- Key files: `execute-phase.md`, `plan-phase.md`, `verify-phase.md`, `transition.md`

**get-shit-done/templates/:**
- Purpose: Reusable templates for planning artifacts
- Contains: Project, milestone, phase, research templates
- Subdirectories: `codebase/` (analysis templates for STACK.md, ARCHITECTURE.md, etc.)

**get-shit-done/references/:**
- Purpose: Documentation and patterns for agents
- Contains: 14 reference markdown files
- Key files: `git-integration.md`, `tdd.md`, `model-profiles.md`, `checkpoints.md`

**get-shit-done/bin/lib/:**
- Purpose: Core JavaScript modules for CLI operations
- Contains: 11 .cjs library files
- Key files: `state.cjs`, `phase.cjs`, `milestone.cjs`, `roadmap.cjs`, `commands.cjs`, `init.cjs`, `core.cjs`

**get-shit-done/templates/codebase/:**
- Purpose: Templates for codebase analysis output
- Contains: 8 analysis template files
- Key files: `stack.md`, `architecture.md`, `structure.md`, `conventions.md`, `testing.md`, `concerns.md`

**agents/:**
- Purpose: Sub-agent definitions for specialized tasks
- Contains: 11 agent markdown files with YAML frontmatter
- Key files: `gsd-executor.md`, `gsd-planner.md`, `gsd-verifier.md`, `gsd-debugger.md`, `gsd-roadmapper.md`

**hooks/:**
- Purpose: Claude Code session hooks
- Contains: 3 JavaScript hook files
- Key files: `gsd-statusline.js`, `gsd-context-monitor.js`, `gsd-check-update.js`

**scripts/:**
- Purpose: Build and utility scripts
- Contains: `build-hooks.js`
- Key files: `build-hooks.js` - Compiles hooks for distribution

**tests/:**
- Purpose: Test suite for core functionality
- Contains: 9 test files (.test.cjs)
- Key files: `init.test.cjs`, `state.test.cjs`, `phase.test.cjs`, `commands.test.cjs`, `helpers.cjs`

**assets/:**
- Purpose: Logo and branding assets
- Contains: SVG and PNG logo files

## Key File Locations

**Entry Points:**
- `bin/install.js`: Main CLI installer (invoked via `npx get-shit-done-cc`)
- `commands/gsd/help.md`: Help command (lists all available commands)

**Configuration:**
- `package.json`: Package manifest with version, dependencies, bin entry
- `get-shit-done/templates/config.json`: GSD configuration template

**Core Logic:**
- `get-shit-done/bin/gsd-tools.cjs`: CLI tool dispatcher
- `get-shit-done/bin/lib/core.cjs`: Core utilities (config loading, output formatting)
- `get-shit-done/bin/lib/state.cjs`: STATE.md operations
- `get-shit-done/bin/lib/phase.cjs`: Phase operations
- `get-shit-done/bin/lib/milestone.cjs`: Milestone operations
- `get-shit-done/bin/lib/roadmap.cjs`: Roadmap operations

**Testing:**
- `tests/helpers.cjs`: Test utilities and fixtures
- `tests/init.test.cjs`: Tests for init command
- `tests/state.test.cjs`: Tests for state operations
- `tests/phase.test.cjs`: Tests for phase operations

## Naming Conventions

**Files:**
- Commands: `kebab-case.md` (e.g., `add-phase.md`, `execute-phase.md`)
- Agents: `gsd-<agent-name>.md` (e.g., `gsd-executor.md`, `gsd-planner.md`)
- Hooks: `gsd-<hook-name>.js` (e.g., `gsd-statusline.js`)
- Libraries: `kebab-case.cjs` (e.g., `state.cjs`, `phase.cjs`)
- Templates: `UPPERCASE.md` for main templates, `kebab-case.md` for subtypes

**Directories:**
- General: `kebab-case/` (e.g., `commands/gsd/`, `get-shit-done/workflows/`)
- Templates: `kebab-case/` or `camelCase/` (e.g., `codebase/`, `research-project/`)

**Frontmatter Fields:**
- `name`: Agent/command identifier (kebab-case for commands)
- `description`: Human-readable description
- `tools`: Comma-separated tool names (e.g., `Read, Write, Bash`)
- `color`: ANSI color name or hex code

## Where to Add New Code

**New Command:**
- Add to: `commands/gsd/<command-name>.md`
- Format: YAML frontmatter + markdown body
- Frontmatter must include: name, description, tools, color

**New Agent:**
- Add to: `agents/gsd-<agent-name>.md`
- Format: YAML frontmatter + markdown with `<role>`, `<execution_flow>` sections
- Must define allowed tools in frontmatter

**New Workflow:**
- Add to: `get-shit-done/workflows/<workflow-name>.md`
- Can be referenced by commands and agents

**New Template:**
- Add to: `get-shit-done/templates/<template-name>.md`
- For codebase analysis: `get-shit-done/templates/codebase/<analysis-type>.md`

**New Reference:**
- Add to: `get-shit-done/references/<reference-name>.md`
- Agents can @-reference these in prompts

**New Core Library:**
- Add to: `get-shit-done/bin/lib/<module-name>.cjs`
- Export functions for: init, state, phase, milestone, roadmap, verify, template
- Register in: `get-shit-done/bin/gsd-tools.cjs` dispatcher

**New Hook:**
- Add to: `hooks/gsd-<hook-name>.js`
- Must follow Claude Code hook interface (stdin JSON, stdout text)

**New Test:**
- Add to: `tests/<module>.test.cjs`
- Use Node.js test runner (`node --test`)
- Use helpers from `tests/helpers.cjs`

## Special Directories

**get-shit-done/templates/codebase/:**
- Purpose: Templates for `/gsd-map-codebase` command output
- Generated: Written to `.planning/codebase/` by gsd-codebase-mapper agent
- Contains: STACK.md, ARCHITECTURE.md, STRUCTURE.md, CONVENTIONS.md, TESTING.md, INTEGRATIONS.md, CONCERNS.md templates

**get-shit-done/bin/lib/:**
- Purpose: Backend logic called by agents via Bash tool
- Generated: Not generated - hand-written CommonJS modules
- Contains: Core state management, phase operations, CLI dispatch

**.planning/ (runtime directory):**
- Purpose: Project planning state created in user's project
- Generated: Created by GSD commands during project work
- Contains: STATE.md, ROADMAP.md, config.json, phases/, milestones/

---

*Structure analysis: 2026-02-21*

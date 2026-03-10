---
gsd_state_version: 1.0
milestone: v2.0
milestone_name: Cross-Platform Launch
status: in_progress
stopped_at: ~
last_updated: "2026-03-10"
last_activity: 2026-03-10 — Milestone v2.0 started, defining requirements
progress:
  total_phases: 0
  completed_phases: 0
  total_plans: 0
  completed_plans: 0
  percent: 0
---

## Current Position

Phase: Not started (defining requirements)
Plan: —
Status: Defining requirements
Last activity: 2026-03-10 — Milestone v2.0 started

## Accumulated Context

### Key Decisions
- Mac support promoted from Out of Scope to v2.0 primary goal
- License key system: LemonSqueezy API (generate + validate keys), in-plugin HTTPS validation via juce::URL
- AU format added alongside VST3 for Logic/GarageBand compatibility
- Ad-hoc signing + xattr quarantine removal for beta tester distribution (no paid Apple Developer account yet)
- Universal binary: arm64 + x86_64 via CMAKE_OSX_ARCHITECTURES

### Blockers
- None

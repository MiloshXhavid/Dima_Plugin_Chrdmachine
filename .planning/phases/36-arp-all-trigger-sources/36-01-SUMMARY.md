---
phase: 36-arp-all-trigger-sources
plan: 01
status: complete
completed: 2026-03-07
---

# Summary: Plan 36-01 — Arp + All Trigger Sources

## What was built

Removed the 5-line force-TouchPlate guard from `processBlock` that overrode Joystick and Random trigger sources to TouchPlate whenever the arp was active. Joystick and Random voices now fire direct notes normally while the arp is on; the existing `isGateOpen → skip` check prevents double notes.

## Key files

### key-files.modified
- `Source/PluginProcessor.cpp` — removed force-TouchPlate block (6 lines deleted)

## Commits

- `59d6d39` — fix(36): remove arp force-TouchPlate guard — Joystick/Random now work with arp active

## Decisions

- Single-site deletion only. No other logic changed — `isGateOpen` skip at ~line 1878 and arp kill-on-disable block already handled all collision/cleanup cases correctly.

## Pending

- Task 2: `checkpoint:human-verify` — build + DAW test (SC1–SC4) awaiting user.

## Self-Check: PASSED

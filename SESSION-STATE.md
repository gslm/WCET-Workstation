# Session State

Source of truth for in-flight state (see DIRECTIVES.md #6). Last updated: 2026-10-05.

## Context

- Operator background: embedded software (C, networking, embedded Linux, several MCUs, IoT). New to time-critical systems and WCET.
- Purpose: self-study of WCET before joining an aircraft software project. Target processor, RTOS and tools are not known yet.
- This workspace runs on the operator's Linux workstations (no MCU target for now).

## Working rules

- `DIRECTIVES.md` applies. It is a verbatim copy of `ai-agents/DIRECTIVES.md` from github.com/gslm/WCET_STM32H755ZI_FreeRTOS, adopted unchanged.
- Key points: do only as told, ask before guessing; checkpoints; concise answers; no subagents/workflows without approval; no AI attribution in commits; ASCII-only in generated docs.

## Decisions

- Theory first, then coding.
- Platform: this Linux workstation, C. QEMU not used for timing (only `-icount` for instruction counts if needed). OTAWA for static analysis later. MCU boards optional, later.
- Layout: one folder per concept, `concepts/NN-name/` with `NOTES-NN-name.md` (theory + results), C source, `Makefile` (output in ignored `build/`). ASCII histograms in C (no matplotlib/gnuplot on dev laptop).
- `concepts/DICTIONARY.md`: terms the operator asks to add. Keep sorted alphabetically; each entry: short definition, description for a beginner, example, why it matters for WCET.
- Repo: github.com/gslm/WCET-Workstation (branch `main`). Operator commits and pushes. `AGENTS.md` to be created as we go.

## Status

- Done: `DIRECTIVES.md`, `STUDY-PLAN.md` (orientation + concept list), this file, `.gitignore`. Pushed to `origin/main`.
- Concept 1 written (`concepts/01-distribution/`), not committed. Awaiting operator review.
- `concepts/DICTIONARY.md` created with 14 terms, not committed.
- Next: concept 2 (measuring correctly), after operator go.

## Resuming on another workstation

Read `DIRECTIVES.md`, `SESSION-STATE.md`, `STUDY-PLAN.md`, then continue from "Next".

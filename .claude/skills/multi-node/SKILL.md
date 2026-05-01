---
name: multi-node
description: QFlex multi-node from the Flexus perspective — Flexus is the clock in the timing phase (cycle count via tick is what becomes virtual time for PDES), Flexus implements pause/resume/is_paused so PDES can halt the timing model, and components/MultiNic + components/NetShim model intra-chip NIC and on-chip network (NOT the inter-machine PDES wire). Use when the user asks about how Flexus drives PDES virtual time, pause/resume on the Flexus side, MultiNic/NetShim, or the features/multi-node branch work.
---

This skill's content lives in `MULTI_NODE.md` next to this directory's `CLAUDE.md`. Read it now: [../../../MULTI_NODE.md](../../../MULTI_NODE.md).

That doc covers Flexus as the clock source, its pause/resume contract with the middleware, and the intra-chip-vs-inter-machine distinction for MultiNic/NetShim, with cross-references to each sibling `MULTI_NODE.md`.

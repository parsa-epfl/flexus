# MULTI_NODE.md — flexus/

This file documents what Flexus contributes to QFlex multi-node and how it relies on the other submodules. The cross-cutting overview is in [../MULTI_NODE.md](../MULTI_NODE.md). Read that first if you haven't.

For this submodule's general context (timing model, build, FLEXUS_API/QEMU_API surface) see [CLAUDE.md](CLAUDE.md).

## What this submodule contributes to PDES

### 1. Flexus is the clock in the timing phase

In phase 4, PDES asks "what virtual time is it?" — the answer comes from Flexus's cycle count, surfaced via the `tick` callback in `QEMU_API_t` (which Flexus calls into the middleware). The middleware translates that into the value PDES reads. Sampling-unit alignment across nodes ultimately rests on this clock being consistent with what the engine expects.

This is a stronger statement than "Flexus reports stats once a unit." Every PDES decision in the timing phase — when to send the next message, when to check for sync, when WWT can advance the quantum, when a snapshot can land — chains back to Flexus's `tick`.

The `features/multi-node` branch's recent work on tick-based cycle counting and multi-node sync primitives is precisely about making this clock contract reliable.

### 2. Flexus implements pause/resume/is_paused

When PDES (via the middleware) decides this node has run past a sync barrier, the middleware calls `FLEXUS_API_t.pause()`. Flexus must stop ticking until `resume()`. `is_paused()` lets the middleware/engine query state without races. These are first-class members of `FLEXUS_API_t` ([../qemu/middleware/libqflex/libqflex-legacy-api.h](../qemu/middleware/libqflex/libqflex-legacy-api.h)) for this exact reason — they are not optional in a multi-node build.

Without Flexus honouring pause, the timing model would race ahead while the engine waits for a neighbour, and virtual time would be wrong by the time the neighbour caught up.

### 3. components/MultiNic/ and components/NetShim/ — intra-chip, not inter-machine

Flexus has [components/MultiNic/](components/MultiNic/) and [components/NetShim/](components/NetShim/). My current best understanding (flag — happy to be corrected):

- These model the **intra-chip** NIC and on-chip network timing — i.e. how packets move between cores, the MMU, and the NIC inside one simulated machine.
- They are **not** the inter-machine PDES wire. PDES lives below the guest NIC, on the QEMU host side; what Flexus sees is the simulated CPU pushing bytes into / pulling bytes out of NIC memory-mapped registers, after PDES has already delivered (or not yet delivered) the packet to the guest.

If multi-node behaviour ever looks like "Flexus's NIC timing model is wrong," the answer is much more likely to be in NetShim/MultiNic than in PDES — and vice versa.

## How this submodule uses the other submodules

- **[../qemu/middleware/](../qemu/middleware/)** — `dlopen`s Flexus from [../qemu/middleware/libqflex/libqflex-module.c:111](../qemu/middleware/libqflex/libqflex-module.c#L111), resolves `flexus_init` at [:118](../qemu/middleware/libqflex/libqflex-module.c#L118), then exchanges the `QEMU_API_t` (Flexus calls these) and `FLEXUS_API_t` (middleware calls these) structs. **All multi-node coordination flows through that one pair of structs.** No other channel.

- **[../qemu/](../qemu/)** — the host process. Flexus runs inside this QEMU's address space; the PDES engine that drives the pause and tick interactions lives in the parent of the middleware (in `net/pdes-*.c`).

- **[../parallel-qemu/](../parallel-qemu/)** — Flexus has no runtime relationship with parallel-qemu. The FW checkpoints WormCache produced *while* parallel-qemu was running are loaded into Flexus at the start of each timing-phase sampling unit, but that handshake goes through the middleware's `snapvm-external` and through Flexus's checkpoint-load path.

- **[../WormCacheQFlex/](../WormCacheQFlex/)** — provides the long-term µarch state Flexus consumes at the start of each sampling unit (cache, TLB, BP). WormCache is multi-node-naive; cross-node coherence of the checkpoints is the engine's job, not Flexus's.

## See also

- [../MULTI_NODE.md](../MULTI_NODE.md) — the comprehensive cross-cutting overview.
- [../qemu/middleware/MULTI_NODE.md](../qemu/middleware/MULTI_NODE.md) — the pause handshake and the Flexus `tick` → virtual time path.
- [../qemu/MULTI_NODE.md](../qemu/MULTI_NODE.md) — the timing QEMU fork Flexus runs inside.
- [../parallel-qemu/MULTI_NODE.md](../parallel-qemu/MULTI_NODE.md) — owner of the canonical PDES code and the FW-phase clock source.
- [../WormCacheQFlex/MULTI_NODE.md](../WormCacheQFlex/MULTI_NODE.md) — produces the FW checkpoints Flexus consumes.

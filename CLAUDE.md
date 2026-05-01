# CLAUDE.md — flexus/

@MULTI_NODE.md

This is a **submodule** of the QFlex simulator. The parent repo lives at `..`; its [CLAUDE.md](../CLAUDE.md) describes the four-phase QFlex pipeline, the `ExperimentContext` model, and how all the pieces fit together. Read it for the full picture; this file only describes what's specific to *flexus/*.

## What this submodule is

The cycle-accurate **microarchitecture timing model**. C++17, built with Conan + CMake, produces shared libraries `libknottykraken.so` / `libsemikraken.so` that Flexus loads into the timing-QEMU process at run time.

It is used in **phase 4 (timing simulation)** only. It is *not* loaded into [parallel-qemu](../parallel-qemu/) and is unrelated to [WormCacheQFlex](../WormCacheQFlex/) — those handle phases 1–3.

## Role in QFlex

- The **timing QEMU** ([../qemu/](../qemu/)) is built with `--enable-libqflex`. The middleware ([../qemu/middleware/libqflex/](../qemu/middleware/libqflex/)) `dlopen`s the Flexus shared object at startup ([libqflex-module.c:111](../qemu/middleware/libqflex/libqflex-module.c#L111)), looks up `flexus_init` ([:118](../qemu/middleware/libqflex/libqflex-module.c#L118)), and hands it a populated `QEMU_API_t` struct.
- During simulation, Flexus drives execution forward instruction-by-instruction (calling QEMU's `cpu_exec` / `tick`). QEMU commits the architectural side; Flexus accumulates timing.
- Flexus consumes:
  - `flexus_configuration.json` — produced from [../templates/flexus_configuration.json.j2](../templates/flexus_configuration.json.j2) per experiment.
  - `timing.cfg` — produced from `templates/timing.cfg.j2`.
  - Per-sampling-unit checkpoints emitted by [WormCacheQFlex](../WormCacheQFlex/) during functional warming, ingested via the parent's `checkpoint_conversion` binary.
- Flexus produces per-cycle measurement output through `core/stats/`, surfaced via QMP commands (see API below).

## Layout

| Path | Purpose |
|---|---|
| `core/qemu/` | The FLEXUS_API / QEMU_API surface. Headers: [api.h](core/qemu/api.h), [trampoline.hpp](core/qemu/trampoline.hpp). Entry point: [startup.cpp](core/qemu/startup.cpp). Sub-APIs: `mai_api.hpp` (memory), `qmp_api.hpp` (QMP), `configuration_api.hpp`. |
| `core/stats/` | Per-cycle measurement collection. `stats.cpp`, `measurement.cpp`, `stats_calc.cpp`. CSV + custom output formats. |
| `core/` (other subdirs) | Simulator framework: `checkpoint/`, `debug/`, `performance/`, `components/` (base classes), `targets/`, `boost_extensions/`, `aux_/`. |
| `components/` | Reusable µarch models (~18): `Cache`, `uFetch`, `Decoder`, `uArch`, `BranchPredictor`, `MMU`, `TLBControllers`, `CMPCache`, `MultiNic`, `NetShim`, `TraceTrackerQEMU`, `MTManager`, `SplitDestinationMapper`, `PhantomCPU`, `MemoryLoopback`, `MemoryMap`, `FetchAddressGenerate`. |
| `target/` | Per-target wiring. Each target has a `wiring.cpp` that picks which components are instantiated and configured: `target/knottykraken/wiring.cpp`, `target/semikraken/wiring.cpp`, plus `nocout-knottykraken`, `nocout-semikraken`, `_profile`. The `SIMULATOR` CMake variable selects which to compile. |
| `conanfile.py`, `CMakeLists.txt`, `CMakeUserPresets.json` | Build config. Conan deps include Boost 1.83.0; build needs GCC 13.1+ for knottykraken. |
| `README.md` | One-line pointer back to the upstream Flexus repo. |

## Build

Driven from the parent's [Makefile](../Makefile):

```sh
make flexus-config                          # initial Conan setup
make flexus-build MODE=release              # or MODE=debug
make flexus-clean-build MODE=release        # also runs `conan cache clean`
```

Build output `.so` files end up at `<parent>/kraken_out/lib{knotty,semi}kraken.so`. From there, `ExperimentContext.set_up_folders()` (in [../commands/config.py](../commands/config.py)) copies them into per-experiment `lib/` directories — but using a **hard-coded absolute path** to `/home/dev/qflex/kraken_out/{libknottykraken.so,libsemikraken.so}` (see [../commands/config.py:319-326](../commands/config.py#L319)). This matches the in-container layout and will fail on a host build.

CMake key compile flags (set in `CMakeLists.txt`): `-DFLEXUS -DTARGET_PLATFORM=aarch64 -DBOOST_MPL_LIMIT_VECTOR_SIZE=50`. Release uses `-O3 -march=native`; Debug uses `-O0 -ggdb`.

## Public interface to the rest of QFlex

All cross-process / cross-submodule glue lives in two function-pointer structs defined in [../qemu/middleware/libqflex/libqflex-legacy-api.h](../qemu/middleware/libqflex/libqflex-legacy-api.h):

- **`FLEXUS_API_t`** — what Flexus exposes to QEMU. Roughly: `start`, `stop`, `qmp` (forwards QMP commands), `pause`, `resume`, `is_paused`, `trace_mem`.
- **`QEMU_API_t`** — what QEMU exposes to Flexus. Roughly: `read_register`, `read_sys_register`, `translate_va2pa`, `get_pc`, `has_irq`, `cpu_exec` (which calls `libqflex_advance` → `libqflex_step`), `tick` (per-cycle from Flexus), `can_stop`, `is_busy`, `stop`, `get_mem`, `disassembly`.

The IPC is **in-process** — no sockets, no shared memory. Flexus lives inside the QEMU process via `dlopen`.

QMP commands Flexus services (see [core/qemu/api.h](core/qemu/api.h)): `QMP_FLEXUS_SETSTATINTERVAL`, `QMP_FLEXUS_WRITEMEASUREMENT`, `QMP_FLEXUS_DOLOAD`, `QMP_FLEXUS_DOSAVE`, `QMP_FLEXUS_SAVESTATS`, `QMP_FLEXUS_TERMINATESIMULATION`. The parent uses these from its run scripts (templated via `templates/run_flexus.sh.j2`).

## Conventions and gotchas

- **All external deps via Conan** — no vendored libraries.
- **Wiring file is the per-target config root.** When adding/removing a component for a target, `target/<name>/wiring.cpp` is the file to edit.
- **Lockstep with parent templates.** When a Flexus configuration field is added or renamed, the parent's template under [../templates/](../templates/) and the corresponding field in `create_experiment_context` ([../commands/config.py](../commands/config.py)) must change together.
- **Hard-coded `/home/dev/qflex/kraken_out/` path** in [../commands/config.py:323](../commands/config.py#L323) — works in the container, breaks on the host.
- **`SIMULATOR` CMake variable** controls which target compiles. The parent Makefile sets this; if invoking CMake directly, you must pass it.
- **`knottykraken` vs `semikraken`**: knottykraken is the full µarch model with all components. semikraken is a smaller variant (notably adds `PhantomCPU` and excludes some components) — used when you want a lighter timing model, e.g. to cohost more cores.
- The submodule lives on branch `features/multi-node` (recent work: tick-based cycle counting, multi-node sync primitives).

## See also

- [../CLAUDE.md](../CLAUDE.md) — qflex root: four-phase pipeline, `ExperimentContext`, build commands.
- [../qemu/CLAUDE.md](../qemu/CLAUDE.md) — timing QEMU fork (this is what loads Flexus via `dlopen`).
- [../qemu/middleware/CLAUDE.md](../qemu/middleware/CLAUDE.md) — the shim that brokers the QEMU↔Flexus connection. Defines the `FLEXUS_API_t` / `QEMU_API_t` structs Flexus implements.
- [../WormCacheQFlex/CLAUDE.md](../WormCacheQFlex/CLAUDE.md) — produces the FW checkpoints Flexus ingests at the start of the timing phase.
- [../parallel-qemu/CLAUDE.md](../parallel-qemu/CLAUDE.md) — fast QEMU used in phases 1–3 (not directly involved in timing, but produces the checkpoints Flexus consumes).

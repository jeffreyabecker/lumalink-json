# C++ Modules Migration Backlog

Purpose: define and sequence the work required to migrate `lumalink-json` from a library-authored header-based API surface to a C++ modules-based API surface, while keeping the shipped semantics, supported spec system, and native test coverage intact.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Current Baseline

The repo is currently a header-only library exposed through `src/LumaLinkJson.h` and the internal headers under `src/json/`.

Current structure and constraints that this backlog must account for:

- The public include surface is an umbrella header, `LumaLinkJson.h`, that includes `json/core.h`, `json/options.h`, `json/traits.h`, `json/spec.h`, `json/scalar_codec.h`, `json/composite_codec.h`, and `json/schema.h`.
- The exported CMake target is `lumalink_json`, declared as an `INTERFACE` library.
- Installation currently copies only `*.h` and `*.hpp` files and exports a header-only package config.
- The compile-fail harness under `test/compile_fail/` invokes the compiler directly with include paths and assumes header inclusion rather than module import.
- The library depends on ArduinoJson headers and vendored PFR headers. The vendored PFR copy already contains module-related conditionals, but the current build does not use them.
- The documented downstream contract is `find_package(LumaLinkJson)` plus `target_link_libraries(... LumaLinkJson::LumaLinkJson)` and `#include <LumaLinkJson.h>`.

Backlog implication: this migration is not just a source rewrite. It changes the build graph, packaging model, consumer entrypoint, test harness assumptions, and likely the minimum supported CMake and compiler versions.

## Phase 0 Decisions

### MOD-00 - Current Header Graph Inventory

The current library-authored surface is organized around one umbrella header and six internal functional headers:

- `src/LumaLinkJson.h`: umbrella include-only entrypoint for consumers
- `src/json/core.h`: core error model, expected aliases, option/state types, and fixed-string support
- `src/json/options.h`: option types and option-selection utilities used by spec descriptors and codecs
- `src/json/traits.h`: enum mapping traits, reflection hooks, and trait-side extension points
- `src/json/spec.h`: spec node types and spec descriptors
- `src/json/scalar_codec.h`: scalar decode and encode path plus core helper templates used by composite code
- `src/json/composite_codec.h`: object, array, tuple, optional, one_of, and reflection-driven traversal
- `src/json/schema.h`: JSON Schema emission path built on the spec graph

The current library-owned dependency chain is:

- `LumaLinkJson.h` depends on all `json/*.h` public headers
- `json/core.h` depends on ArduinoJson and `json/traits.h`
- `json/options.h` depends on `json/core.h`
- `json/spec.h` depends on `json/options.h` and `json/traits.h`
- `json/scalar_codec.h` depends on `json/core.h`, `json/spec.h`, and `json/traits.h`
- `json/composite_codec.h` depends on `json/scalar_codec.h` and vendored PFR headers
- `json/schema.h` depends on ArduinoJson plus `json/core.h`, `json/options.h`, `json/spec.h`, and `json/traits.h`

Important migration constraints surfaced by the current graph:

- The code is not just a flat public surface; `core`, `options`, `traits`, `spec`, codecs, and schema each carry distinct responsibilities that map naturally to partitions or implementation units.
- `json/composite_codec.h` currently relies on prior inclusion through `json/scalar_codec.h`, so module conversion must remove that textual-include assumption and make dependencies explicit.
- ArduinoJson is used directly in `core.h` and `schema.h`, not only in implementation detail code, so the module boundary must deliberately bridge a third-party header dependency.
- Vendored PFR is used only from the composite path today, which argues for keeping reflection support behind a non-exported boundary unless a real public need appears.
- Tests currently validate the include-based surface: runtime tests link the `INTERFACE` target and compile sources normally, while compile-fail tests invoke the compiler directly with include roots for `src`, vendored PFR, and ArduinoJson.
- Installation and package export assume copied headers are the shipped contract, which is incompatible with a module-first end state unless the packaging model changes.

Resulting Phase 0 inventory conclusion:

- the repo needs one deliberate public module entrypoint
- the internal layering should remain an implementation concern rather than becoming public module structure
- the build, install, and test contracts all need explicit redesign; a file-by-file syntax rewrite is not sufficient

### MOD-01 - Canonical Module Topology Options

The viable topology options for this repo are:

- Option A: one public primary module, `lumalink.json`, with internal partitions for `core`, `options`, `traits`, `spec`, `scalar_codec`, `composite_codec`, and `schema`
- Option B: several peer public modules such as `lumalink.json.core`, `lumalink.json.spec`, `lumalink.json.codec`, and `lumalink.json.schema`
- Option C: one public primary module with no meaningful partitioning, implemented as one large interface unit plus implementation units

Tradeoffs:

- Option A matches the current logical decomposition, keeps the consumer surface small, and lets the repo hide ArduinoJson and PFR decisions behind one import path.
- Option B maps more directly to the current header layout, but it recreates public fragmentation, makes export hygiene harder, and forces downstream consumers to understand internal layering that is currently hidden behind `LumaLinkJson.h`.
- Option C gives the simplest downstream story, and it still fits this repo as long as the existing layering stays inside implementation units rather than becoming public partitions.

Decision for MOD-01:

- choose Option C as the canonical target shape
- keep `lumalink.json` as the only public consumer-facing module
- keep the current internal layering as implementation detail rather than public topology
- keep PFR-related code out of exported surface area and treat it as a hidden implementation detail for now

### MOD-02 - Compatibility Decision

Decision: do not add compatibility headers, shims, aliases, forwarding includes, or any other compatibility layer for the old header-based consumer model.

Implications of this decision:

- `#include <LumaLinkJson.h>` stops being a supported migration path once the module cutover lands.
- The repo should delete or retire library-authored public headers as part of the module migration instead of keeping them as a second consumer surface.
- Documentation, tests, install rules, and examples should move directly to the module consumer path rather than documenting parallel include and import workflows.
- Any code changes required for migration must produce one canonical contract, not an include-based fallback.

## Phase 1 Decisions

### MOD-10 - Canonical Compiler Version Floor

Decision:

- Windows canonical compiler floor: MSVC toolset 14.38 or newer
- Linux canonical compiler floor: GCC 15 or newer
- Canonical generator for both: Ninja 1.11 or newer

Rationale:

- CMake's standard module support starts in CMake 3.28, and its documented module scanning support covers MSVC 14.34+ and GCC 14+.
- MSVC has the most mature mainstream named-module workflow in this set, and 14.38 is a safer practical floor than 14.34 because it avoids choosing the earliest CMake-supported toolset during a migration that will already stress packaging and install behavior.
- GCC 14 is the first CMake-supported GCC line for module scanning, but GCC's own module documentation still describes the implementation as incomplete. Choosing GCC 15 as the project floor is the more defensible practical baseline for a library migration rather than targeting the first minimally supported release.
- This backlog does not require `import std`, so the compiler floor does not need to be driven by standard-library module support. The library should plan around named modules plus global-module-fragment header bridging where needed.

Workflow implications:

- Windows support should target Visual Studio 2022 with an MSVC toolset at or above 14.38.
- Linux support should target a GCC 15 toolchain rather than attempting to preserve compatibility with GCC 13 or GCC 14 for the module-based workflow.
- If a future implementation pass proves GCC 15 still too unstable for this codebase, the fallback should be to raise the GCC floor again, not to reintroduce header-mode compatibility.

Resulting MOD-10 conclusion:

- do not target pre-14.38 MSVC for the module migration
- do not target pre-15 GCC for the module migration
- do not depend on `import std` for the initial cutover

### MOD-11 - Canonical CMake Version And Target Model

Decision:

- Raise the project CMake floor from 3.25 to 3.30 or newer for the module migration branch
- Treat CMake 3.28 as the absolute feature floor, but not the project support floor
- Use CMake-managed named-module support through `FILE_SET`s of type `CXX_MODULES`
- Keep Ninja as the canonical generator on both supported host platforms

Canonical target model:

- replace the current header-only `INTERFACE` target shape with a real library target that owns the module interface unit and any implementation units
- publish the primary module interface from a `PUBLIC` `CXX_MODULES` file set
- keep implementation units as normal target sources, not public module surface
- keep third-party header bridging in the global module fragment or implementation-unit includes as needed, rather than trying to model third-party headers as project-owned modules
- continue exporting a single downstream target, `LumaLinkJson::LumaLinkJson`, even if its implementation stops being header-only

Rationale:

- CMake's standard C++ modules support was added in 3.28, so the current 3.25 baseline is not viable for a modules-based migration.
- This repo needs more than basic scanning. It needs a build that can model module sources explicitly, preserve correct ordering, and eventually support install and export behavior for a downstream package.
- Choosing 3.30 instead of 3.28 avoids anchoring the project to the first release with module support. That is the safer baseline for a library migration that will depend on target export and packaging behavior rather than just local compilation.
- Ninja remains the canonical generator because CMake's module support and related documentation are strongest there, and this repo already uses Ninja on Linux. Keeping one generator family reduces avoidable variation while the build graph is changing.
- The current `INTERFACE` target shape is a poor fit for a module-first library because the project will need target-owned module sources and build-time module metadata. Preserving the old target shape would optimize for the old packaging model instead of the new one.

Workflow implications:

- `cmake_minimum_required` and `CMakePresets.json` must both move to a module-capable baseline as part of the migration.
- Module interface units must be known to CMake at configure time and attached to the owning target explicitly.
- The repo should not attempt a bespoke BMI-management workflow in scripts when CMake already owns module scanning, collation, install, and export behavior.
- The initial module cutover should avoid Visual Studio generator-specific behavior and center on Ninja-based presets for both Windows and Linux.

Resulting MOD-11 conclusion:

- CMake 3.25 is out of scope for the module migration
- CMake 3.28 is the technical minimum, but 3.30 is the recommended project floor
- the library should migrate to a real target with a public `CXX_MODULES` file set instead of trying to preserve a pure header-only `INTERFACE` target

### MOD-12 - Exported Target And Install Rules Redesign

Decision: replace the header-only `INTERFACE` target and header-copy install with a `STATIC` library target whose module interface unit is published through a `PUBLIC CXX_MODULES` file set and installed as a module source.

Library target changes:

- Change `add_library(lumalink_json INTERFACE)` to `add_library(lumalink_json STATIC)`
- Attach the primary module interface unit with `target_sources(lumalink_json PUBLIC FILE_SET CXX_MODULES BASE_DIRS src FILES <interface-unit>)` — the `PUBLIC` visibility causes the file set to propagate through the install config to downstream consumers
- Attach implementation units as ordinary `PRIVATE` target sources, not via the file set
- Move the vendored PFR include path from `INTERFACE` to `PRIVATE` — it was only ever an implementation detail; the module boundary now enforces that
- Keep `target_compile_features(lumalink_json PUBLIC cxx_std_23)` so the language standard propagates to consumers
- The `INTERFACE` include directory that pointed at `src/` is removed from the exported contract; consumers get the module surface only and do not need a raw include root

Install rules changes:

- Remove the `install(DIRECTORY src/ DESTINATION include ...)` call — no library-authored headers are shipped in the install tree
- Add `FILE_SET CXX_MODULES DESTINATION modules/lumalink_json` to the `install(TARGETS ...)` call so the interface source lands at a stable install-relative path
- Add `ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}` to the same `install(TARGETS ...)` call so the compiled static archive is installed
- Add `CXX_MODULES_DIRECTORY cxx-modules` to `install(EXPORT LumaLinkJsonTargets ...)` so CMake emits the companion `*CXXModules.cmake` files that downstream `find_package` consumers need to wire up the module source on their imported target

Resulting install tree layout:

- `lib/lumalink_json.lib` (Windows) or `lib/liblumalink_json.a` (Linux) — compiled archive
- `modules/lumalink_json/<interface-unit>` — module interface source consumed by downstream builds
- `lib/cmake/LumaLinkJson/LumaLinkJsonConfig.cmake` — package config
- `lib/cmake/LumaLinkJson/LumaLinkJsonVersion.cmake` — version config
- `lib/cmake/LumaLinkJson/LumaLinkJsonTargets.cmake` — exported target definitions
- `lib/cmake/LumaLinkJson/LumaLinkJsonTargets-<config>.cmake` — per-config target properties
- `lib/cmake/LumaLinkJson/cxx-modules/LumaLinkJsonTargets-CXXModules.cmake` — module source companion
- `lib/cmake/LumaLinkJson/cxx-modules/LumaLinkJsonTargets-<config>-CXXModules.cmake` — per-config module companion

Package config changes:

- The `LumaLinkJsonConfig.cmake.in` template must include the generated CXX module companion file so downstream consumers get the module source attached to the imported target automatically after `find_package(LumaLinkJson)`
- `find_dependency(ArduinoJson)` remains in the config for as long as ArduinoJson types appear in the exported module interface; MOD-31 will determine whether the public API can be restructured to remove this requirement from consumers
- The config no longer sets any include directories on the consumer; the module file set replaces that role

Rationale:

- A `STATIC` target is the correct shape for a library that compiles real object code from implementation units. An `INTERFACE` target cannot own compiled sources or a `CXX_MODULES` file set.
- Making the `CXX_MODULES` file set `PUBLIC` and installing it with a `FILE_SET` destination is the CMake-standard way to ship a source-only module distribution. It is the mechanism that makes the MOD-13 decision (source-only install, no BMIs) actually work for downstream consumers.
- `CXX_MODULES_DIRECTORY` on `install(EXPORT ...)` is the required hook for CMake to generate the companion config files. Without it the installed package cannot wire up the module source on the imported target.
- Removing the PFR include path from `INTERFACE` closes the accidental leak of a vendored implementation detail that was reachable by any downstream consumer who happened to transitively include the library. The module boundary makes this enforcement structural rather than advisory.

Resulting MOD-12 conclusion:

- `lumalink_json` becomes a `STATIC` target with a `PUBLIC CXX_MODULES` file set
- installed package ships the module interface source and the compiled archive, not copied headers
- `install(EXPORT ...)` must use `CXX_MODULES_DIRECTORY` to emit the module companion config files
- `LumaLinkJsonConfig.cmake.in` must include the companion file and remove the include-directory contract

### MOD-13 - BMI Artifact Handling Strategy

Decision: install the module interface unit source only; do not ship pre-built BMI files in the install tree.

Canonical install model:

- The primary module interface unit (`.ixx` or `.cppm`) is installed to a known location and attached to the exported `LumaLinkJson::LumaLinkJson` target via a `PUBLIC CXX_MODULES` file set in the CMake config
- Downstream consumers compile their own BMI from the installed interface source using their own compiler, toolset version, and flags
- No pre-built BMI files are staged or shipped; no `install-cxx-module-bmi-*.cmake` collate scripts are included in the install tree
- CI validates the install-then-consume path by doing a fresh out-of-source configure against the installed package, not by reusing build-tree artifacts

Rationale:

- BMIs are not portable across compiler versions, flag sets, or operating systems. Shipping them introduces a silent correctness risk: a consumer whose compiler or flags differ from the package producer will get link-time or import-time failures that are hard to diagnose.
- The CMake collate-based BMI install mechanism is Ninja-specific and does not work for consumers using the Visual Studio generator. Shipping BMIs would therefore create an asymmetric install tree that only partially works across the supported matrix.
- Installing the interface source and letting the consumer's build system produce its own BMI is the correct model for a general-purpose distributed library. It is also the model CMake's standard module support is designed around at the consumer side.
- This decision pairs with MOD-12: the exported CMake config must expose the interface source through a `CXX_MODULES` file set so CMake can compile it as part of the consumer's build rather than as a raw include file.

Workflow implications:

- The install tree layout needs one published interface source file and a generated CMake config that declares it as a `CXX_MODULES` source on the imported target.
- The `find_package` + `target_link_libraries` consumer workflow remains the documented downstream path; the consumer does not need to do anything special to trigger BMI compilation.
- CI should treat a clean install + downstream build as a first-class validation scenario rather than only testing the build-tree workflow.

Resulting MOD-13 conclusion:

- do not ship BMIs in the install tree
- do not generate or include collate-step BMI install scripts
- publish the interface unit source through the CMake config so downstream consumers compile their own BMI automatically

## Design Intent

This backlog targets a canonical module-first library surface with these outcomes:

- Consumers can use `import lumalink.json;` as the primary entrypoint instead of `#include <LumaLinkJson.h>`.
- The library-owned API surface is authored as one public module interface plus implementation units instead of public `.h` files.
- Internal implementation layering remains explicit and stable rather than collapsing into one oversized interface unit.
- CMake exports and installs the module surface in a reproducible way for supported toolchains.
- Existing runtime and compile-fail test coverage remains meaningful after the migration.

The intended target shape is:

- one public primary module: `lumalink.json`
- one primary interface unit that exports the full public library contract
- any remaining layering belongs in implementation units, not public partitions
- no compatibility headers or alternate consumer shims; the module surface is the only supported library-owned entrypoint after cutover

## Scope

- Library-owned public API migration from headers to C++ module interface units
- Internal dependency and layering changes needed to make the module graph compile cleanly
- CMake target, install, and package export changes required for module-aware consumption
- Test harness updates required so runtime tests and compile-fail tests validate the module surface
- Documentation and CI updates required to make the new workflow reproducible

## Non-Goals

- Do not redesign the spec system, error model, or JSON semantics as part of the module migration
- Do not module-ize ArduinoJson or the C++ standard library beyond whatever import/include bridging is required for this library to compile
- Do not replace vendored PFR unless module integration proves it is a hard blocker
- Do not expose vendored PFR as part of the public module topology; keep it hidden behind implementation detail
- Do not preserve the current pure `INTERFACE` target shape if a compiled or module-bearing target is required
- Do not add compatibility headers, import shims, aliases, or forwarding include layers for the old consumer contract
- Do not expand this backlog into unrelated performance or API cleanup work unless a compile blocker forces a narrowly scoped change

## Architectural Rules

- Treat `import lumalink.json;` as the canonical end-state consumer entrypoint.
- Preserve the current namespace and top-level API names unless a module constraint forces a change that is explicitly documented.
- Prefer one public primary module interface over exposing many peer public modules or public partitions.
- Keep library-owned includes out of the canonical consumer path once the migration is complete.
- Do not ship parallel compatibility layers for the prior include-based API; there should be one library-owned consumer contract.
- Keep third-party headers behind module boundaries where practical, using a global module fragment or other supported bridging model rather than leaking dependency-specific include requirements into consumer code.
- Keep vendored PFR behind non-exported implementation detail so downstream consumers do not depend on it.
- Update the build and package model to the smallest shape that actually works; do not force the old header-only packaging contract onto a module-based library if that makes the result fragile.

## Work Phases

## Phase 0 - Inventory And Decision Record

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| MOD-00 | done | Inventory the current library-authored header graph, public entrypoints, and transitive dependency edges that must move behind a module surface | none | A written inventory identifies the current public headers, internal headers, third-party includes, and test assumptions that depend on textual inclusion |
| MOD-01 | done | Decide the canonical module topology: one public primary module interface with implementation units and no public partitions | MOD-00 | The backlog and follow-on implementation have a stable module graph instead of ad-hoc file-by-file conversion |
| MOD-02 | done | Decide the compatibility policy for migration consumer surfaces | MOD-01 | The backlog explicitly forbids compatibility headers, shims, aliases, and other fallback consumer layers |

## Phase 1 - Toolchain, CMake, And Packaging Foundation

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| MOD-10 | done | Define the minimum supported compiler versions and exact module workflow per platform for MSVC and GCC, including any limitations that affect canonical support | MOD-01 | The repo documents a supported module-capable compiler matrix instead of assuming current header-mode support carries over |
| MOD-11 | done | Decide the minimum CMake version and target model needed for standard C++ modules support, including whether the project must move beyond CMake 3.25 | MOD-10 | The build system has a credible module-aware foundation and the required CMake bump is explicit if needed |
| MOD-12 | done | Redesign the exported target and install rules so the package can publish module interface units and any required compiled artifacts or metadata | MOD-11 | A downstream consumer can discover and link the library through CMake without relying on copied public headers as the primary contract |
| MOD-13 | done | Define how generated build artifacts such as BMIs are treated in local builds, install trees, and CI so the package contract stays reproducible | MOD-12 | The package and CI strategy does not rely on undocumented compiler-cache state |

## Phase 2 - Public Module Surface

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| MOD-20 | done | Create the primary `lumalink.json` module interface and map the current umbrella-header surface into exported declarations | MOD-01, MOD-11 | A downstream consumer can import the public module and access the current public API without including `LumaLinkJson.h` |
| MOD-21 | done | Split implementation across non-exported implementation units where that reduces compile coupling and keeps the primary interface readable | MOD-20 | The module graph stays maintainable without turning the internal layers into public partitions |
| MOD-22 | done | Define which current declarations remain exported, which become implementation detail, and how that maps to the current `json/*.h` structure | MOD-20, MOD-21 | Export visibility is documented and enforced by module boundaries instead of accidental header reachability |
| MOD-23 | done | Remove the umbrella header and any remaining library-authored public include entrypoints once the module surface is in place | MOD-20, MOD-22, MOD-02 | The repo exposes the module surface only and no longer ships a parallel include-based library contract |
| MOD-24 | todo | Update native runtime tests to import the module and use module-based entrypoints instead of header includes | MOD-20, MOD-22 | The native test suite validates the module consumer path rather than only the legacy include-based path |
| MOD-25 | todo | Redesign the compile-fail harness so negative-case sources compile as module consumers and still report expected failures cleanly through CTest | MOD-20, MOD-12 | Compile-fail cases exercise the module surface without depending on raw include-directory injection |

### MOD-22 - Export Visibility Policy

Decision: promote all library-owned declarations to public export except those directly coupled to vendored PFR logic.

Export visibility mapping by current header:

- `core.h`: all declarations are public. The error model, `expected<>`, option types, and fixed-string support are all part of the library's API contract and should remain exported.
- `options.h`: all declarations are public. Option types and selection utilities are used by spec descriptors and the public codec interfaces.
- `traits.h`: all declarations are public. Enum mapping traits and reflection hooks are legitimate extension points that downstream code should access.
- `spec.h`: all declarations are public. Spec node types and descriptors are the core schema-definition API.
- `scalar_codec.h`: all declarations are public. Scalar encode and decode are standalone paths that consumers may call directly for simple types; the core helper templates are not leaky implementation details.
- `composite_codec.h`: split visibility.
  - Type definitions, public codec functions (`encode_composite`, `decode_composite`), and traversal helpers that work without reflection are public.
  - PFR-specific reflection hooks (`refl_encode_composite`, `refl_decode_composite`) and any PFR-coupled template specializations are kept as non-exported implementation detail. These are implementation mechanisms, not part of the documented schema API.
- `schema.h`: all declarations are public. JSON Schema emission is a documented feature and consumers already call these paths.

Implementation strategy:

- The primary interface unit exports all public declarations through inline function declarations, type aliases, and namespace-scope `export` blocks.
- PFR reflection code stays in an implementation unit and is never re-exported through the interface.
- Consumers have no visibility into the vendored PFR namespace or its include paths. They call the library's public reflection API (the `traits` and encoded schema output), not the reflection library directly.

Rationale:

- The library's current public surface (pre-module) already exposed all of these types and functions through `LumaLinkJson.h`. The module migration should not artificially narrow the API unless a hidden dependency forces a restructuring.
- PFR is different: it is a vendored dependency that was never intended as part of the public contract. The old header model leaked it through `#include` chains, but the module boundary should not carry that leak forward.
- By exporting everything except PFR, the module surface stays compatible with the current API surface, reducing downstream migration friction.

Resulting MOD-22 conclusion:

- export all `core`, `options`, `traits`, `spec`, `scalar_codec` (full), `composite_codec` (public paths only), and `schema` declarations
- keep PFR-specific composite codec paths as non-exported implementation detail
- no module partitions; everything is part of the single `lumalink.json` interface or implementation units

### MOD-20 - Primary Interface Unit Implementation

Decision: create a single primary interface unit `src/lumalink.json.ixx` that re-exports the full public API by including internal implementation units and bundling all exported declarations in namespace `lumalink` and `lumalink::json`.

Concrete file structure:

- `src/lumalink.json.ixx`: primary module interface unit
  - Global module fragment to `#include <ArduinoJson.h>` (MOD-31 may refine this)
  - `import` statements for each internal implementation module (core, traits, options, spec, codecs, schema)
  - `export namespace lumalink { ... }` blocks that re-export types and functions from the implementation modules
  - Inline wrapper functions where needed to provide a single cohesive API surface
- `src/lumalink.json-impl-core.ixx`: private implementation unit for core types
- `src/lumalink.json-impl-traits.ixx`: private implementation unit for trait types and extension hooks
- `src/lumalink.json-impl-options.ixx`: private implementation unit for option types
- `src/lumalink.json-impl-spec.ixx`: private implementation unit for spec node types
- `src/lumalink.json-impl-scalar.ixx`: private implementation unit for scalar encode/decode
- `src/lumalink.json-impl-composite.ixx`: private implementation unit for composite codec (public paths only; PFR paths stay here)
- `src/lumalink.json-impl-schema.ixx`: private implementation unit for JSON Schema paths
- `src/lumalink.json-impl-pfr.ixx`: private implementation unit for PFR-specific reflection utilities (never re-exported)

Implementation approach:

- The primary interface unit is minimal and acts as a facade. It does not define the actual types or functions; it `import`s them from implementation units and re-exports them through namespace-scope `export` blocks.
- Each internal implementation unit maps directly to a current header's logical layer. Internal includes between implementation units (e.g., impl-options depending on impl-core) are allowed since they are all `internal`; the dependency graph is not public.
- The global module fragment handles the ArduinoJson bridge. Types like `::ArduinoJson::JsonDocument` or `::ArduinoJson::JsonValue` must be available to the implementation; they are not directly exported to consumers but are used in function signatures that are exported (MOD-31 will verify this is feasible).
- PFR includes stay in `src/lumalink.json-impl-pfr.ixx` which is only imported by `src/lumalink.json-impl-composite.ixx`. The PFR paths never leak into the export surface.

Testing and validation:

- Confirm that `import lumalink.json;` compiles and does not require any additional includes.
- Verify that all currently public declarations (from the MOD-22 inventory) are reachable through the `lumalink` and `lumalink::json` namespaces.
- Compile a simple downstream test that uses each category of API (encode/decode, spec definition, traits, schema emission) to confirm the module surface is complete.

Resulting MOD-20 conclusion:

- create the primary interface unit as a facade that imports and re-exports from internal implementation units
- establish the mapping between current headers and internal implementation modules
- no public partitions; all implementation details including PFR are hidden from consumers

### MOD-21 - Implementation Unit Organization

Decision: organize the implementation units so internal dependencies stay clean, each layer can be understood independently, and PFR concerns remain isolated.

Concrete dependency graph for internal implementation units:

```
lumalink.json (primary interface, facade)
  ├─ imports: impl-traits, impl-core, impl-options, impl-spec, impl-scalar, impl-composite, impl-schema
  └─ re-exports all public declarations

impl-core
  └─ includes: <ArduinoJson.h>, <string>, <optional>, stdlib

impl-traits
  └─ imports: impl-core
  └─ includes: stdlib

impl-options
  └─ imports: impl-core, impl-traits
  └─ includes: stdlib

impl-spec
  └─ imports: impl-options, impl-traits
  └─ includes: stdlib

impl-scalar
  └─ imports: impl-core, impl-spec, impl-traits
  └─ includes: stdlib

impl-composite
  └─ imports: impl-scalar, impl-spec
  └─ imports (conditionally): impl-pfr (for reflection paths only)
  └─ includes: stdlib

impl-schema
  └─ imports: impl-core, impl-options, impl-spec, impl-traits, impl-scalar
  └─ includes: <ArduinoJson.h>, stdlib

impl-pfr (completely internal, never imported by the interface)
  └─ imports: impl-spec, impl-traits
  └─ includes: pfr headers, stdlib
  └─ exports: internal helper types and functions only (no `export` namespace)
```

Benefits of this structure:

- Acyclical internal import graph. No circular dependencies between implementation units.
- Each layer is independently understandable: core types, then traits, then options/spec, then codecs, then schema.
- PFR is a side module that only composite code imports. If reflection is disabled, impl-pfr is never compiled.
- Consumers see only a single entry point (`import lumalink.json;`) and have no visibility into the internal structure.

Compile strategy:

- CMake configuration attaches all `src/lumalink.json-impl-*.ixx` files to the target as `PRIVATE` sources (not in a file set).
- The primary `src/lumalink.json.ixx` is attached as the `PUBLIC` file set (the one that downstream consumers import).
- CMake's module scanning automatically detects the `import` statements in the interface unit and builds the internal dependency graph.

Testing requirements:

- Confirm that moving from header-based textual inclusion to module import does not break any of the internal cross-layer function calls.
- Verify that PFR-heavy code paths (composite + reflection) compile and link correctly with the internal impl-pfr module but remain unreachable from the public interface.
- Compile a downstream consumer and confirm it does not accidentally gain visibility into impl-pfr despite composite_codec using it internally.

Resulting MOD-21 conclusion:

- each implementation unit corresponds to a logical layer in the current header graph
- internal module dependencies are acyclical and correspond to current header dependency structure
- PFR is completely isolated in a side module that is never re-exported
- all internal implementation units are `PRIVATE` target sources; only the primary interface is public

### MOD-23 - Umbrella Header Retirement

Decision: delete `src/LumaLinkJson.h` and remove any remaining library-authored public include entrypoints once the module interface is complete and test harness is migrated.

Files to delete:

- `src/LumaLinkJson.h`: the old umbrella header

CMake changes:

- Remove the `target_include_directories(...INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>...)` line from the target, since the module boundary replaces the include-root contract.
- If any CMakeLists.txt or test configuration still references the old include directories for header-based consumption, remove those references and rely on module imports instead.
- The `src/libs/pfr` include path should already have been removed from the public interface in MOD-12; if not, confirm it is `PRIVATE`.

Documentation and examples:

- Update README, usage-guide, and any code examples to show `import lumalink.json;` as the only entrypoint.
- Remove any examples that use `#include <LumaLinkJson.h>` or `#include <json/...>`.
- If version guides or upgrade documentation exist, note that the include-based API is no longer available in this version.

Test harness validation:

- Confirm MOD-24 and MOD-25 have fully migrated the test suite before deleting the umbrella header.
- Run the full test suite to verify that no orphaned header includes remain in test code.

CI validation:

- Rebuild and retest the library after header deletion to confirm there are no stale internal dependencies on the deleted file.
- Use a clean clone to validate that the install tree and downstream consumption both work without the old headers present.

Rationale:

- Keeping the old header around as a "compatibility layer" violates the MOD-02 decision (no compatibility layers). Deleting it enforces the module-only contract structurally.
- The module interface is the canonical API surface; the old headers become noise and maintenance risk if they linger.
- Downstream consumers have explicit, traceable failure (missing include) if they try to use the old header; this is better than a confusing dual-contract where both styles "work" temporarily.

Resulting MOD-23 conclusion:

- delete `src/LumaLinkJson.h` and any remaining public header files
- confirm test harness is module-based before deletion
- update documentation and examples to reflect module-only consumption
- use CI to validate that no stale includes remain in the codebase

### MOD-24 - Runtime Test Migration

Decision: convert all native runtime test sources to import the `lumalink.json` module instead of including `LumaLinkJson.h` and internal headers.

Test source updates:

- Each test source file currently contains header includes like `#include "../src/LumaLinkJson.h"` and may include ArduinoJson or other dependencies.
- Replace all these with a single `import lumalink.json;` statement at the top.
- If a test directly uses ArduinoJson types (e.g., in test assertions or fixture setup), keep the `#include <ArduinoJson.h>` for now; MOD-31 will determine if the public module interface can avoid exporting ArduinoJson types.
- Remove all includes of internal headers (`json/core.h`, `json/spec.h`, etc.); these are no longer in the public contract and should not be referenced from test code.

CMake configuration updates:

- Update `test/CMakeLists.txt`: remove the `target_include_directories(... $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src> ...)` lines since module imports replace direct include-path injection.
- Verify that tests still link against the `LumaLinkJson::LumaLinkJson` target; CMake's module scanning will wire up the import automatically.
- If the test configuration has a `PRIVATE` include directory for vendored PFR, remove it (PFR is now an implementation detail).

Validation:

- Recompile the test suite and verify all tests build and pass.
- Confirm that the test binary includes the right BMI files (should be auto-generated by CMake).
- Run the test executable to ensure the module imports work at runtime.

Rationale:

- The module is the canonical consumer API; tests should validate the documented usage pattern, not rely on internal header details.
- By moving tests to module imports early, we prove that the module interface is complete and accessible before finalizing the public surface.
- This also simplifies maintenance: if an internal header changes, tests fail early rather than silently relying on internal structure.

Resulting MOD-24 conclusion:

- all native test sources use `import lumalink.json;` as the entrypoint
- no internal header includes remain in test code
- test CMakeLists.txt removes all direct include-directory configuration
- test suite still links against the target normally; module imports are automatic

### MOD-25 - Compile-Fail Harness Redesign

Decision: convert the compile-fail test framework to validate module-based negative cases instead of raw compiler invocations with include paths.

Current harness behavior (pre-migration):

- `test/compile_fail/` contains source files that are expected to fail compilation.
- A CMake script (`cmake/run_compile_fail_test.cmake`) or inline CTest custom commands invoke the compiler directly with include-path arguments.
- The harness captures compiler output and checks for expected error patterns.

New harness approach:

- Keep the same compile-fail sources but rewrite them to use `import lumalink.json;` instead of includes.
- Instead of raw compiler invocation, use `try_compile()` in CMake to attempt module compilation and capture the failure.
- Or, continue raw compiler invocation but include the proper module scanning and BMI generation flags that the build system generates.

Compile-fail test source updates:

- Each `test/compile_fail/*.cpp` file should have `import lumalink.json;` at the top instead of `#include` statements.
- The test source should attempt to use the API in a way that should fail (e.g., invalid type combinations, using private functions if possible, etc.).
- Rewrite existing negative cases to target module-level failures (missing exports, wrong namespace, etc.) rather than header-specific errors.

CMake harness updates:

- Update `cmake/run_compile_fail_test.cmake` to:
  - Extract the module interface unit's path from the target's `CXX_MODULES` file set.
  - Run the compiler with the appropriate module-scanning and BMI-generation flags (e.g., `-std=c++23 -fmodules -fmodule-mapper=...` for GCC, or `/std:c++latest /experimental:module` for MSVC).
  - Point to the build-tree module interface source and any required include paths (e.g., ArduinoJson for bridging).
  - Capture the compiler's stderr and check for expected error messages.
- Consider using CMake's `try_compile()` as an alternative: it handles the module infrastructure automatically, though it may hide some compiler diagnostics.

Validation:

- Run the compile-fail test suite and verify that expected failures still fail with sensible error messages.
- Ensure no false positives (code that should fail but compiles).
- Confirm that the error messages are clear enough for the test regex patterns to match correctly.

Rationale:

- The compile-fail harness should validate the module contract, not legacy header behavior.
- Raw compiler invocation with include paths will not work once headers are removed; the harness must adapt to module scanning.
- By keeping compile-fail validation early, we catch API misdesigns before accepting the module interface.

Resulting MOD-25 conclusion:

- all compile-fail sources use `import lumalink.json;`
- the harness invokes the compiler with proper module scanning and BMI generation
- expected failure patterns are validated through module-aware compilation
- error diagnostics remain clear and testable

## Phase 3 - Internal Dependency Migration

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| MOD-30 | todo | Refactor library-owned cross-header dependencies so implementation units do not rely on cyclic textual inclusion patterns | MOD-21 | The module graph compiles without include-order sensitivity or hidden textual coupling |
| MOD-31 | todo | Integrate ArduinoJson through the chosen module-bridging approach and verify that parse, encode, and schema entrypoints remain valid | MOD-20, MOD-30 | Third-party dependency usage is compatible with the public module surface and does not leak new consumer requirements |
| MOD-32 | todo | Evaluate the vendored PFR path under modules and keep it behind non-exported implementation detail unless a hard blocker forces a different approach | MOD-30 | Reflection support compiles under the chosen module model without undocumented compiler-specific hacks and without becoming public module surface |
| MOD-33 | todo | Remove or shrink library-authored public headers once the module graph carries the real API surface | MOD-23, MOD-31, MOD-32 | The repo no longer depends on the old header layout as the canonical authoring model |

## Phase 4 - Test Harness Migration

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| MOD-40 | todo | (Moved to MOD-24) | | |
| MOD-41 | todo | (Moved to MOD-25) | | |
| MOD-42 | todo | Add targeted regression cases for module-specific failure modes such as missing exports, implementation-unit wiring mistakes, and consumer import order assumptions | MOD-24, MOD-25 | The suite covers failures unique to module-based delivery rather than only legacy header behaviors |

## Phase 5 - Consumer Migration, Docs, And CI

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| MOD-50 | todo | Update README and usage documentation so the primary setup and examples use `import lumalink.json;` only | MOD-20, MOD-23 | Documentation reflects the module-only entrypoint and no longer presents header inclusion as a supported library workflow |
| MOD-51 | todo | Update install and downstream-consumption documentation to describe the supported CMake workflow for module consumers on Windows and Linux | MOD-12, MOD-13 | A clean downstream consumer can follow the documented module consumption path without extra tribal knowledge |
| MOD-52 | todo | Add CI coverage for configure, build, and test of the module workflow across the supported compiler matrix | MOD-40, MOD-41, MOD-50 | CI proves the module build and test path on each supported platform/toolchain combination |
| MOD-53 | todo | Audit release and package outputs to confirm no compatibility headers or include-based entrypoints are still shipped | MOD-23, MOD-50, MOD-52 | Install and release artifacts expose only the intended module-based consumer contract |

## Phase 6 - Validation And Exit Criteria

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| MOD-60 | todo | Validate a clean Windows module consumer flow from clone to configure, build, test, install, and downstream import | MOD-52 | The Windows module workflow is reproducible without undocumented manual fixes |
| MOD-61 | todo | Validate a clean Linux module consumer flow from clone to configure, build, test, install, and downstream import | MOD-52 | The Linux module workflow is reproducible without undocumented manual fixes |
| MOD-62 | todo | Audit the repo for stale header-first assumptions in docs, tests, install rules, examples, and package metadata after the module cutover | MOD-50, MOD-51, MOD-53 | Remaining header references are removed or justified as third-party implementation detail rather than shipped library contract |
| MOD-63 | todo | Define the criteria for considering the module migration complete under a strict module-only consumer model | MOD-60, MOD-61, MOD-62 | The backlog states exact technical, packaging, and documentation conditions for closing the migration |

## Sequencing Notes

- Start with the build and packaging decisions. A module migration that ignores CMake version and compiler support will fail late.
- Treat the current `INTERFACE` target as an implementation detail, not a requirement to preserve.
- Keep the public module graph minimal: one primary module interface, with remaining layering pushed into non-public implementation units.
- Move the test harness early enough that compile-fail coverage does not silently keep validating the wrong consumer model.
- Do not spend work on migration shims; move the docs, package, and tests directly to the module-only contract.

## Source References

- `CMakeLists.txt`
- `src/LumaLinkJson.h`
- `src/json/core.h`
- `src/json/options.h`
- `src/json/traits.h`
- `src/json/spec.h`
- `src/json/scalar_codec.h`
- `src/json/composite_codec.h`
- `src/json/schema.h`
- `src/libs/pfr/pfr/config.hpp`
- `README.md`
- `docs/usage-guide.md`
- `test/CMakeLists.txt`
- `test/compile_fail/`
<p align="center">
  <img src="https://kirzo.dev/content/images/plugins/ScriptableFramework_banner.jpg" alt="ScriptableFramework Banner" width="512">
</p>

<h1 align="center">ScriptableFramework</h1>

<p align="center">
  <em>A data-driven gameplay framework for Unreal Engine — composable Tasks &amp; Conditions, event-driven visual <strong>Scriptable Graphs</strong>, automatic property bindings, reusable assets, and a polished node-graph editor with live debugging.</em>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Unreal%20Engine-5.x-blue?logo=unrealengine" alt="Unreal Engine 5.x" />
  <img src="https://img.shields.io/badge/language-C%2B%2B20-00599C?logo=c%2B%2B" alt="C++20" />
  <img src="https://img.shields.io/badge/Blueprint-ready-orange" alt="Blueprint ready" />
  <img src="https://img.shields.io/badge/license-MIT-green.svg" alt="MIT License" />
  <img src="https://img.shields.io/github/stars/kirzo/ScriptableFramework?style=social" alt="GitHub stars" />
</p>

---

## Table of Contents

- [Overview](#overview)
- [Plugins &amp; Modules](#plugins--modules)
- [Feature Tour](#feature-tour)
  - [Tasks &amp; Actions](#tasks--actions)
  - [Conditions &amp; Requirements](#conditions--requirements)
  - [Context &amp; Locals](#context--locals)
  - [Property Bindings](#property-bindings)
  - [Reusable Assets](#reusable-assets)
  - [Scriptable Graphs](#scriptable-graphs)
    - [Node Types](#node-types)
    - [Running &amp; Cancelling Graphs](#running--cancelling-graphs)
    - [Events &amp; Wireless Jumps](#events--wireless-jumps)
    - [The Graph Editor](#the-graph-editor)
    - [Debugging: Breakpoints &amp; Live Trace](#debugging-breakpoints--live-trace)
    - [Compile &amp; Validation](#compile--validation)
  - [Async &amp; Fire-and-Forget Execution](#async--fire-and-forget-execution)
  - [Built-in Tasks](#built-in-tasks)
  - [Built-in Conditions](#built-in-conditions)
  - [StateTree Integration](#statetree-integration)
  - [Gameplay Ability System Integration](#gameplay-ability-system-integration)
  - [Level Sequence Integration](#level-sequence-integration)
  - [UMG / Widget Integration](#umg--widget-integration)
  - [Editor Tooling](#editor-tooling)
- [Requirements](#requirements)
- [Installation](#installation)
- [Repository Layout](#repository-layout)
- [Usage Examples](#usage-examples)
- [Related Projects](#related-projects)
- [Contributing](#contributing)
- [License](#license)
- [Author](#author)

---

## Overview

**ScriptableFramework** is an open-source gameplay plugin suite for **Unreal Engine 5** that lets designers and programmers build complex behaviour from small, composable, data-driven blocks: **Tasks**, **Actions**, **Conditions** and **Requirements**. When flat lists aren't enough, you graduate the very same building blocks into **Scriptable Graphs** — an event-driven, pin-based visual editor (Blueprint-style exec wires, no ticking) with sub-graphs, breakpoints and live debugging.

It comes with a sophisticated **property bindings system** (Context, Locals, Sibling, Function, Auto-bindings, pluggable **Value Converters**), reusable **Asset workflows** (Action assets, Requirement assets, Graph assets), **wrapper nodes** to embed assets and inline blocks, **deep editor integration** (custom pickers, validators, drag-and-drop UI, copy/paste, graph diff), and out-of-the-box integrations with **StateTree**, **Gameplay Abilities**, **Level Sequences** and **UMG**.

It is built on top of **[KzLib](https://github.com/kirzo/KzLib)** — leveraging its `FKzParamDef`, `FKzNamedVariant`, `FInstancedPropertyBag` helpers, asset-editor toolkit and Slate widgets (including the shared validation panel) — so you get a battle-tested foundation without reinventing the wheel.

The framework is **modular**, **Blueprint-friendly**, and designed to scale from simple gameplay scripts (e.g. *"wait 2 seconds, then spawn a particle"*) to full event-driven behaviour graphs with shared parameters, mutable local variables, and runtime-driven logic.

---

## Plugins & Modules

ScriptableFramework ships as a **family of independent Unreal plugins** so you can pick exactly what your project needs. Each integration sits in its own `.uplugin` and depends on the core plugin being enabled.

| Plugin | Modules | Enabled by default | Purpose |
| :--- | :--- | :---: | :--- |
| **`ScriptableFramework`** | `ScriptableFramework` (Runtime), `ScriptableFrameworkConverters` (Runtime), `ScriptableFrameworkUncooked` (UncookedOnly), `ScriptableFrameworkEditor` (Editor) | ✅ | Core: ScriptableObject, Tasks, Actions, Conditions, Requirements, **Scriptable Graphs**, Bindings + Converters, Context, Locals, Async, and the full editor toolkit + graph editor. |
| **`ScriptableFrameworkAI`** | `ScriptableFrameworkAI` (Runtime) | 🔲 | StateTree integration — `Run StateTree` task with dynamic parameter binding. |
| **`ScriptableFrameworkGAS`** | `ScriptableFrameworkGAS` (Runtime) | 🔲 | Gameplay Ability System — granting abilities, sending events, tag management. |
| **`ScriptableFrameworkSequencer`** | `ScriptableFrameworkSequencer` (Runtime) | 🔲 | Level Sequence — play sequences, wait for finish, set playback position. |
| **`ScriptableFrameworkUI`** | `ScriptableFrameworkUI` (Runtime) | 🔲 | UMG — create / remove widget tasks. |

Inside the core plugin:

- **`ScriptableFramework`** (Runtime) — all the runtime types (containers, tasks, conditions, graphs, bindings). Depends only on Unreal Engine, `PropertyBindingUtils` and KzLib.
- **`ScriptableFrameworkConverters`** (Runtime) — houses `UScriptableValueConverter` subclasses that teach the binding system how to copy between non-identical types (e.g. an `AActor*` into an `FKzTransformSource`). Kept separate so converters can pull in engine types the core doesn't need.
- **`ScriptableFrameworkUncooked`** (UncookedOnly) — the custom `UK2Node`s that give the Context setters a chainable pin flow in Blueprint graphs. Lives here so cooked builds never pull in `BlueprintGraph` / `KismetCompiler`.
- **`ScriptableFrameworkEditor`** (Editor) — the whole editor experience: customizations, pickers, validators, and the Scriptable Graph asset editor. Editor builds only; never enters cooked builds.

Optional plugins are disabled by default; enable only the ones you need.

---

## Feature Tour

### Tasks &amp; Actions

**`UScriptableTask`** is the unit of execution. Tasks have a status (`None` / `Begun` / `Finished`), latent execution, and four overridable hooks: `BeginTask`, `FinishTask`, `ResetTask`, plus `StopTask` for interruption. Both C++ and Blueprint subclasses are first-class:

- **Control settings (`FScriptableTaskControl`)**:
  - `bLoop` + `LoopCount` (0 = infinite) — task automatically restarts on `Finish()`.
  - `bDoOnce` — task only ever runs once during its lifetime.
- **Native &amp; Dynamic delegates** for `OnTaskBegin` / `OnTaskFinish`, both BP-assignable and C++-bindable.
- **`IsStoppable()`** — tasks that own in-flight latent work (timers, sequence playback, sub-graphs) advertise a `Stop` pin so a graph can interrupt them cleanly; atomic tasks opt out.

**`FScriptableAction`** is a container struct holding a list of tasks, an execution `Mode` (Sequence or Parallel), a shared **Context**, and mutable **Locals**. Actions are the entry point most users hit:

- **`Sequence`** — tasks run one after another. The action finishes when the last one finishes.
- **`Parallel`** — all tasks start simultaneously. The action finishes when every task has finished.
- **`Run(Owner)`** registers all sub-tasks, injects the binding map, context and locals, and kicks off execution.
- **`Run(MoveTemp(Action), Owner)`** — fire-and-forget static overload (see [Async &amp; Fire-and-Forget Execution](#async--fire-and-forget-execution)).
- **`Reset()`** halts execution and propagates a hard reset to every task (great for revertible side effects).
- **`Clone(NewOuter)`** creates a deep copy of the action and all sub-tasks — used internally to instantiate runtime copies of action assets.

<p align="center">
    <img src="https://kirzo.dev/content/images/plugins/ScriptableFramework/ScriptableFrameworkTasks.gif" width="768">
</p>

### Conditions &amp; Requirements

The same composable design powers boolean logic:

- **`UScriptableCondition`** — a single boolean check. Subclasses override `Evaluate_Implementation()`. Supports `bNegate` to invert the result; `CheckCondition()` resolves bindings, evaluates, and applies negation.
- **`FScriptableRequirement`** — a container of conditions with a `Mode` (`And` / `Or`), a `bNegate` flag for the whole group, and an optional shared `Context`. `EvaluateRequirement(Owner, Requirement)` is the static one-shot entry point.
- **Stable evaluation semantics**: empty `And` requirements return `true`; empty `Or` requirements return `false`. Algorithms use `Algo::AllOf` / `Algo::AnyOf` for clarity and short-circuiting.
- **Nested groups**: `UScriptableCondition_NestedRequirement` embeds a full `FScriptableRequirement` inside another requirement — perfect for `(A AND B) OR (C AND D)` expressions in the UI (it inherits the parent scope's context rather than declaring its own).

<p align="center">
    <img src="https://kirzo.dev/content/images/plugins/ScriptableFramework/ScriptableFrameworkConditions.gif" width="768">
</p>

### Context &amp; Locals

Every container (`FScriptableAction`, `FScriptableRequirement`), asset and graph can declare two flavours of shared memory:

**Context** — a typed input parameter bag, the interface a scope exposes to the outside world:

- **`ContextDefinitions`** — a `TSet<FKzParamDef>` declaring the schema (name + type per parameter). Powered by KzLib's `FKzParamDef`, so you get the full editor pin-type picker with arrays, structs, enums, soft references, etc.
- **`Context` (`FInstancedPropertyBag`)** — the runtime memory backing the schema, rebuilt on demand from the definitions (transient — always go through `GetContext()`).
- **Templated accessors**: `AddContextProperty<T>`, `SetContextProperty<T>`, `GetContextProperty<T>` (KzLib-backed) for type-safe C++ usage.
- **Blueprint setters with custom thunks**: `SetActionContextParameter` / `SetRequirementContextParameter` / `SetGraphInstanceContextProperty` are wildcard-pin nodes that accept any value type and runtime-validate it against the schema (with safe failure and `KismetExecutionMessage` warnings on type mismatch).
- **Chainable Context authoring**: `MakeScriptableContext` mints a context from a single typed value; the custom **`UK2Node_SetScriptableContextProperty`** / **`UK2Node_AddScriptableContextProperty`** nodes preserve a clean visual Context In → Context Out chain on top of the otherwise non-chainable `void + UPARAM(Ref)` setters.
- **Inheritance**: containers automatically fall back to the parent scope's context if their own is empty, enabling hierarchical scoping.

**Locals** — per-instance **mutable** state (a lightweight blackboard), new alongside the graph system:

- **`LocalsDefinitions`** — a `TArray<FKzNamedVariant>` (name + type + default value). Seeded into a runtime `Locals` bag when the scope registers.
- Written at runtime by **`Set Local`** and **`Call Function`** tasks, and read anywhere via the bindings **Locals** scope.
- **Write-through inheritance**: nested wrappers (Nested Action / Nested Requirement / Sub-Graph) share the parent scope's Locals so inner tasks read and write the same variables without redeclaring them.

### Property Bindings

This is where ScriptableFramework really shines. The bindings system lets any property on a Task, Condition or Node pull its value from somewhere else at runtime:

- **`FScriptablePropertyBindings`** — list of `FScriptablePropertyBinding` entries (each with a Source path, Target path, Source Guid, and `bIsAutoBinding` flag).
- **Path-based resolution** — `FPropertyBindingPath` describes traversal segments (property name + optional array index). The custom `ResolveIndirections` walks the path manually so it can dive into:
  - Standard struct members and array elements.
  - **`FInstancedPropertyBag` payloads** anywhere in the chain (e.g. binding to a dynamic StateTree parameter, a Context variable, or a Call Function's arguments bag).
  - **UFunctions mid-path** — pure / const Blueprint-callable functions that take no parameters and return a value (e.g. `GetActorLocation().X`).
- **Source scopes**:
  - **Context** — read from the nearest scope's Context bag.
  - **Locals** — read from the nearest scope's mutable Locals bag.
  - **Sibling** — read a previous task / condition / node's output, resolved via the `BindingSourceMap` by persistent GUID.
  - **Cross-node Outputs** — in a graph, any other node's exposed Output property.
- **Type coercion** at resolve time: identical types use a fast `CopyCompleteValue`; the system also handles `UObject` ↔ `TObjectPtr`, child-class to parent-class casts, `Object → Bool` (validity check), `Numeric ↔ Numeric`, `Bool ↔ Numeric`, and `Byte ↔ Enum`.
- **Pluggable Value Converters** — when a source and target aren't natively coercible, the binding system consults **`UScriptableValueConverter`** subclasses. A converter lists the `(from → to)` type pairs it handles and performs the copy. Converters are stateless and auto-discovered (the CDO does the work), so any module can teach the system a new conversion just by declaring one — e.g. `UKzTransformSourceConverter` turns an actor / scene component / vector / rotator / transform into an `FKzTransformSource` target.
- **Auto-bindings** — properties marked with metadata `meta = (ScriptableContext)` (or `Category = "Context"`) are automatically wired to a compatible variable in scope. Re-baked on every save and editor change (templates only) so renames / type changes are caught immediately.
- **Property categorization metadata** — `ScriptableInput`, `ScriptableOutput`, `ScriptableContext` (or matching `Category` prefixes) drive editor pills (`IN` / `OUT` / `CONTEXT`) and validation rules.
- **Sanitization** — `SanitizeObsoleteBindings` removes broken bindings on load; array element removal/clear automatically shifts indices in any binding pointing into that array; single-entry Context/Locals renames redirect every referencing binding.

<p align="center">
    <img src="https://kirzo.dev/content/images/plugins/ScriptableFramework/requirement_2.gif" alt="Binding a property to a Context variable via the inline picker" width="768">
</p>

### Reusable Assets

Actions, Requirements and Graphs all have first-class asset variants for reuse and modularity:

- **`UScriptableActionAsset`** — a `Const` data asset wrapping an `FScriptableAction`. Supports a `MenuCategory` (e.g. `Combat|Melee`) that powers the editor picker organization.
- **`UScriptableRequirementAsset`** — same idea for requirements.
- **`UScriptableGraph`** — the graph asset (see [Scriptable Graphs](#scriptable-graphs)).
- **Wrapper nodes** make assets and inline blocks usable inside any list:
  - **`UScriptableTask_RunAsset`** — runs a `UScriptableActionAsset` as a sub-task, deep-copying its tasks into a transient runtime instance and inheriting the parent context.
  - **`UScriptableCondition_Asset`** — same idea for requirement assets.
  - **`UScriptableTask_NestedAction`** — wraps an *inline* `FScriptableAction` so task-level control (loop, do-once) applies to a whole sub-flow (e.g. *"play dialogue + wait, looping"*).
  - **`UScriptableCondition_NestedRequirement`** — wraps an inline `FScriptableRequirement` for grouped logic.
- **Context propagation**: wrappers pass their parent context (and Locals) down to the embedded asset/block, so an asset declaring a `Target` parameter is automatically fed by the outer scope.
- **Asset Registry tags**: `MenuCategory` is exposed as an alphabetical asset registry tag so the type picker can surface it instantly.

### Scriptable Graphs

When a flat list of tasks isn't enough, **Scriptable Graphs** let you author event-driven flow visually. A graph is a network of nodes connected by **execution wires** — Blueprint-style pins where a "pulse" travels along wires between named output → input pins. Nodes are purely event-driven: **there is no per-frame tick**; a node reacts when one of its input pins fires.

A **`UScriptableGraph`** asset stores:

- **`Nodes`** — every node in the graph (instanced, owned by the asset).
- **`Connections`** — a single flat list of `FScriptableGraphConnection { From, To }`, each endpoint addressed by `(NodeID, PinName)`. Wires live centrally on the asset, not on the nodes.
- **`EntryNodeID`** — the always-present, undeletable Entry node that fires when the graph launches.
- **`Context`** and **`Locals`** — the same input schema + mutable variables as any other scope.
- **`Outputs`** — user-declared completion pins, appended after the Exit node's built-in `Finished` / `Cancelled` outputs and mirrored by any Sub-Graph node that references this asset.
- **Debug metadata** — `Breakpoints`, per-node and asset-wide **trace levels**, and the editor-only visual `EdGraph`.

<p align="center">
    <img src="https://kirzo.dev/content/images/plugins/ScriptableFramework/sg_overview.jpg" width="768">
</p>

#### Node Types

| Node | Role |
| :--- | :--- |
| **Entry** | Unique start node. Fires its single output the moment the graph launches. Cannot be deleted. |
| **Task** | Wraps a single `UScriptableTask`. Inputs: `Start` (+ `Stop` if the task is stoppable); outputs come from the task's own pin set. This is how every built-in and custom task drops into a graph. |
| **Sequence** | Fans one input out to N ordered outputs (`Then 0..N`), fired in order. Add / remove output pins inline. Mirrors Blueprint's Sequence. |
| **Branch** | Evaluates an inline `FScriptableRequirement` and fires `True` or `False`. |
| **Switch** | Ordered list of requirement "cases"; fires the first passing case, else `Default`. Exactly one output per activation. |
| **AND** | Joins N inputs into one output; fires once every input has pulsed (accumulates across frames), then resets. |
| **OR** | Fires on the first input pulse, then latches — a one-shot "first wins" join. |
| **Receive Event** | External-trigger entry point (no inputs). Fires when a matching named event is raised on the runner. Renameable in place (F2). |
| **GoTo** | "Wireless" jump: raises a target event, waking matching Receive Event nodes without a drawn wire. |
| **Sub-Graph** | Runs another `UScriptableGraph` inline; its output pins mirror the referenced asset's Exit pin set. Double-click to open the child. |
| **Finish** | Terminator: stops in-flight work and routes completion through the Exit node's chosen output. |
| **Exit** | Optional cleanup endpoint (at most one). The runner fires one of its outputs (`Finished` / `Cancelled` / a custom Output) right before teardown — a guaranteed cleanup sub-flow. |
| **Reroute** | Pure visual knot to organize wires; created by double-clicking a wire. No effect on flow. |

Nodes are colour-coded by role (Tasks blue, Conditions red, System nodes dark, Entry green, Exit red) and pick up the wrapped task's dynamic display title, so a graph reads at a glance before you click into anything.

#### Running &amp; Cancelling Graphs

Graphs are launched and kept alive by a world-scoped **`UScriptableGraphSubsystem`**, which owns the live runner instances so they survive while they execute and can all be cancelled safely on world teardown (PIE end, level change).

- **From Blueprint** — the async **`Run Scriptable Graph`** node (`UAsyncRunScriptableGraph`): pass an owner, the graph, a launch `Context`, and an optional run `Id`. `Started` fires immediately with the live `UScriptableGraphInstance` (use it to send events or mutate context while it runs); `Finished` fires when the graph completes.
- **From C++** — `UScriptableGraph::Run(Graph, Owner, Context, Id)` → `UScriptableGraphSubsystem::RunGraph(...)`.
- **As a sub-execution** — the `Run Graph` task (`UScriptableTask_RunGraph`) runs a graph from inside an Action, and the `Sub-Graph` node does the same from inside another graph.

Under the hood, a **`UScriptableGraphInstance`** deep-copies the asset's nodes (GUIDs survive duplication, so baked bindings still resolve), reads the connections as immutable lookup data, and pumps a re-entrant activation queue driven entirely by pin-fire delegates — no ticking. When no active work remains, it runs the Exit cleanup sub-flow (if any) and tears itself down.

**Cancellation** is first-class (Blueprint-callable on the subsystem or via the `UScriptableBlueprintLibrary` helpers):

- `CancelRunner(Runner)` / `Cancel()` — cancel a specific run.
- `CancelRunnersById(Id)` — cancel every run launched with a given `Id`.
- `CancelRunnersForOwner(Owner)` / `CancelScriptableRunnersForOwner(...)` — cancel everything a given object started.
- `CancelAllRunners()` / `CancelAllScriptableRunners(World)` — cancel every live runner (graphs *and* action runners). Graphs run their Exit cleanup; action runners force-finish.

#### Events &amp; Wireless Jumps

Events are plain named `FName` signals scoped to a single runner — no gameplay-tag indirection, no global bus:

- **`Runner->FireEvent(EventName)`** (Blueprint-callable) wakes every `Receive Event` node whose name matches, in parallel. Anyone holding the live runner (from `Started`, or `GetActiveRunners()`) can raise events on it.
- **GoTo** nodes raise an event internally, so you can jump to any `Receive Event` node without drawing a wire — ideal for state-machine-style flows and keeping large graphs readable.

#### The Graph Editor

Opening a Scriptable Graph asset launches a full node-graph editor toolkit:

- **Panels** — a **Graph** canvas, an **asset Details** panel, a **Node Details** panel (Task nodes are unwrapped to edit the inner task directly), a drag-out **Palette** (all tasks + native nodes), a **Validation** panel, and a **Search** panel.
- **Add-node menu** — right-click the canvas (or drag off a pin) to get the same searchable, categorized type picker used across the framework, organized into *Scriptable Tasks*, *Native Nodes* and *Scriptable Nodes*. Picking a node auto-wires it to the pin you dragged from.
- **Drag &amp; drop from the Content Browser** — drop an **Action asset** to get a `Run Asset` task node, a **Graph asset** to get a `Sub-Graph` node, or a **Task Blueprint** to get a Task node — validated without loading the assets.
- **Editing polish** — full Copy / Cut / Paste / Duplicate, comment boxes, node **Search** (`Ctrl+F`) with click-to-navigate and matched-text fragments, **align &amp; distribute** commands, **delete-and-reconnect** (heals wires across a removed node), inline **"+ Add pin"** buttons on Sequence / AND / OR, per-pin "Remove pin", and BP-style **orphan pins** (a vanished runtime pin stays visible and red until rewired).
- **Reroute knots** — double-click any wire to splice in a reroute control point.
- **Quick-spawn shortcuts** — hold **S / B / G / E / A** and click the canvas to drop a Sequence / Branch / GoTo / Event / Action node at the cursor.
- **Convert to Sub-Graph** — select a region and refactor it into a brand-new graph asset in one action; inbound wires become event inputs, outbound wires become outputs.
- **Toolbar** — a dynamic **Compile** button (colour reflects dirty / failed / ok), **Home** (centre on Entry), **Clean Graph** (prune everything unreachable from Entry or any Receive Event), **Search**, and a **Debug Object** picker.
- **Diff** — source-control "Diff against revision" opens a dedicated side-by-side Scriptable Graph diff viewer.

#### Debugging: Breakpoints &amp; Live Trace

Scriptable Graphs debug like Blueprints:

- **Breakpoints** — toggle with **F9** (or the node context menu). Enabled/disabled breakpoints show an overlay icon; hitting one during PIE pauses execution and jumps the editor to the node.
- **Live active-node halo** — pick a running instance from the toolbar **Debug Object** combo and watch the currently-active node light up with an orange halo as the pulse travels the graph.
- **Trace levels** — per-node and asset-wide trace verbosity, overridable by the `scriptable.TraceLevel` console variable, for logging the flow without a debugger.

#### Compile &amp; Validation

Graphs run a **compile** pass that persists a pass/fail flag on the asset — a runtime **launch is refused while the last compile failed**, so a broken graph never ships a half-run. Two auto-discovered validators feed the in-editor Validation panel and inline node banners:

- **Structural** (`UScriptableGraphValidator`) — cycles, duplicate node IDs, empty/duplicate event names, connections to missing nodes, inverted connections, missing pins, unreachable nodes, self- and indirect graph recursion, multiple Exit nodes, unset GoTo targets, invalid Outputs, and more.
- **Bindings** (`UScriptableBindingsValidator`) — unbound required Inputs, unresolvable Context bindings, and stale/obsolete bindings, each with a one-click **Clear** quick-fix.

Every issue is **click-to-navigate**: selecting it in the Validation panel selects and pans to the offending node (issues carry the node's `BindingID`), and each node shows an inline ERROR!/WARNING! banner. The Validation tab label carries a live count badge.

### Async &amp; Fire-and-Forget Execution

ScriptableFramework offers several ways to run logic without manually managing its lifetime:

- **`UAsyncRunScriptableAction`** — a `UBlueprintAsyncActionBase` node that gives Blueprints a clean async/await-style API for Actions. It registers with the game instance, hooks into `OnActionFinish`, broadcasts the `OnFinish` BP delegate, and auto-cleans the action via `Reset()` on completion.

- **`UAsyncRunScriptableGraph`** — the `Run Scriptable Graph` node described above, with `Started` (live runner) and `Finished` events.

- **`FScriptableAction::Run(FScriptableAction&& Action, UObject* Owner)`** — a static fire-and-forget overload for C++. The action is moved into an internal `UScriptableActionRunner` (a hidden UObject) which keeps a self-reference alive until the action finishes, guaranteeing stable memory addresses for all internal `AddRaw` bindings. If the runner is destroyed mid-run (owner died, world tearing down), it force-finishes the action so child tasks unregister cleanly.

```cpp
// Fire-and-forget — caller doesn't need to keep the action around.
FScriptableAction::Run(MoveTemp(MyAction), this);

// vs. the instance form when you need handles for Reset / IsRunning / etc.
MyMemberAction.Run(this);
```

### Built-in Tasks

The core module ships a useful starter set:

- **`UScriptableTask_LogMessage`** — prints to log and/or screen with selectable severity (`Info`/`Warning`/`Error`), color, and duration. Smart defaults: when `TextColor` stays at the default Cyan, severity dictates the color (yellow for Warning, red for Error).
- **`UScriptableTask_Wait`** — latent timer-based wait. Supports `Duration` plus an optional `RandomDeviation` (`Duration ± Random`). Falls back to immediate completion if no world is available (e.g. asset-editor preview).
- **`UScriptableTask_SetLocal`** — writes a value into one of the scope's Locals slots and finishes immediately. The target is a dropdown driven by the owning asset's Locals; the value is type-constrained to the picked local.
- **`UScriptableTask_CallFunction`** — calls any `BlueprintCallable` function on a target object via reflection (the side-effecting complement to pure-function binding sources). Its `Parameters` bag mirrors the function signature — set constants inline or bind members — and the return value can be written into a Local for downstream nodes to read.
- **`UScriptableTask_RunGraph`** — runs a `UScriptableGraph` asset as a sub-execution.
- **`UScriptableTask_NestedAction`** — runs an inline `FScriptableAction` as a single looping/do-once unit.

Built-in tasks expose dynamic display titles so bound parameters show up directly in the editor list (`Wait "Duration" s`, `Log: "MyMessage"`, `Set "Health"`, …).

### Built-in Conditions

A complete logic toolkit out of the box:

- **`UScriptableCondition_Bool`** — checks a single bool. Display title respects bindings and negation: `IsAlive`, `!IsAlive`, `Is True`, `Is False`.
- **`UScriptableCondition_CompareNumbers`** — typed comparison (`==`, `!=`, `<`, `<=`, `>`, `>=`) with configurable `ErrorTolerance` for float equality.
- **`UScriptableCondition_CompareBooleans`** — `AND`, `OR`, `XOR`, `NAND`, `==`, `!=`.
- **`UScriptableCondition_Distance`** — squared-distance check between two actors with the same set of comparison operators.
- **`UScriptableCondition_Probability`** — `0..1` chance, useful for non-deterministic gameplay (`30% Chance`).
- **`UScriptableCondition_IsValid`** — null/garbage-collection check on any UObject reference.
- **`UScriptableCondition_NestedRequirement`** — nested requirement with its own mode and negation, inheriting the parent context. Renders inline like a fold-out group in the editor.

### StateTree Integration

`ScriptableFrameworkAI` adds a single but very powerful task:

- **`UScriptableTask_RunStateTree`** — finds a `UStateTreeComponent` on the target actor (prioritizing the Controller for Pawns, falling back to the actor itself), assigns a `FStateTreeReference` and starts it. Supports `bForceRestart` to safely interrupt a running tree.
- **Dynamic StateTree parameter binding** — `PreResolveBindings` scans the bindings on the task, identifies which ones target the StateTree's parameter bag, and marks each one as `SetPropertyOverridden(true)` *before* the StateTree's sync pass — so your bound values aren't wiped by the engine's overwrite logic. The editor side surfaces those dynamic parameters as proper bindable rows under a `Dynamic StateTree Parameters Bindings` group.

### Gameplay Ability System Integration

`ScriptableFrameworkGAS` ships ready-made building blocks for projects using GAS:

- **Tasks**:
  - `UScriptableTask_GrantAbility` — grants a Gameplay Ability with optional dynamic source tags. Supports `bRevertOnReset` to revoke the granted ability via the stored `FGameplayAbilitySpecHandle`.
  - `UScriptableTask_ActivateAbilityByTag` — natively activates abilities by `FGameplayTag` via `TryActivateAbilitiesByTag`.
  - `UScriptableTask_ManageGameplayTag` — `Add` or `Remove` loose Gameplay Tags on the target ASC, with `bRevertOnReset` to flip the operation on reset.
  - `UScriptableTask_SendGameplayEvent` — dispatches a fully-populated `FGameplayEventData` (tag, magnitude, instigator, target, two optional UObjects) to an actor's ASC.
- **Conditions**:
  - `UScriptableCondition_HasAbility` — checks whether the ASC has a `TSubclassOf<UGameplayAbility>` granted, optionally requiring it to be currently active.
  - `UScriptableCondition_CanActivateByTag` — asks GAS whether any ability matching a given tag passes its full activation gate (tags, costs, cooldowns).
  - `UScriptableCondition_MatchTagQuery` — runs a `FGameplayTagQuery` against the actor's owned tags (via `IGameplayTagAssetInterface` or its ASC).

All nodes have rich editor display titles so logic reads naturally even before you click into them: `Grant Ability [Sprint] to Player`, `Send Event [Damage.Hit] to Target`, `Does Player have Ability [Dash]`, …

### Level Sequence Integration

`ScriptableFrameworkSequencer` plugs Level Sequences into the framework with three latent tasks:

- **`UScriptableTask_PlayLevelSequence`** — calls `Play()` on a `ULevelSequencePlayer`. With `bWaitUntilFinished = true` the task only finishes after `OnFinished` fires; otherwise it kicks off playback and finishes immediately. Cleanup is defensive: the `OnFinished` binding is removed both on natural completion and on external `Finish()` (e.g. if the action is cancelled mid-play).
- **`UScriptableTask_WaitForLevelSequenceFinish`** — pure waiter: subscribes to `OnFinished` on a player that's already playing and finishes when the sequence ends. Skips waiting if the player is null or already idle.
- **`UScriptableTask_SetLevelSequencePlaybackPosition`** — synchronously sets the playback position of a player using a full `FMovieSceneSequencePlaybackParams` (frame, evaluation type, jump behavior, …) and finishes immediately.

Combined with the rest of the framework, you can sequence cinematics with gameplay logic naturally: *grant ability → wait sequence → send event → run another asset*, all in one Action or Graph.

### UMG / Widget Integration

`ScriptableFrameworkUI` adds two immediate UMG tasks (finish the same frame):

- **`UScriptableTask_CreateWidget`** — instantiates a `UUserWidget` and optionally adds it to the viewport or the player's screen (split-screen aware), with a `ZOrder`. Exposes the created widget as an **Output** so sibling tasks can bind to it.
- **`UScriptableTask_RemoveWidget`** — removes a widget from its parent. Wire its `Widget` input to a sibling's output (e.g. `Create Widget`).

Together they make show/hide HUD flows a two-node graph: *Create Widget → (gameplay) → Remove Widget*, the second bound to the first's output.

### Editor Tooling

Beyond the graph editor, `ScriptableFrameworkEditor` is a sizeable toolkit for the list-based (Action / Requirement) workflows:

- **Asset workflow**:
  - Asset factories for `UScriptableActionAsset`, `UScriptableRequirementAsset` and `UScriptableGraph` under the **Scriptable Framework** Content Browser category (Action blue, Requirement red, Graph teal).
  - Custom thumbnails, icons and category colors registered via `FScriptableFrameworkEditorStyle` (KzLib's `TKzEditorStyle_Base` CRTP).

- **Property customizations** — every container, task, condition and group has its own `IPropertyTypeCustomization`:
  - **`FScriptableContainerCustomization`** (base) — header with title, mode pill (`AND`/`OR`, `Sequence`/`Parallel`), Add button (with full type picker), Remove-all button, Context / Locals editor popups.
  - **`FScriptableObjectCustomization`** (base) — header with checkbox, dynamic title, type picker, Use-Selected/Browse/Edit buttons, options menu (Replace With, Copy, Paste, Duplicate, Remove, Clear).
  - **`FScriptableTaskCustomization`** — adds `Once` and `Loop` toggle pills with inline `LoopCount` editor.
  - **`FScriptableConditionCustomization`** — adds `NOT` toggle pill.
  - Dedicated customizations for Nested Action / Nested Requirement, Set Local (typed value editor), Call Function (function picker + auto-synced parameters bag), and the concrete Action / Requirement containers.

- **Reusable Slate widgets**:
  - **`SScriptableTypePicker`** / **`SScriptableTypeSelector`** — searchable, hierarchical type/asset picker reused by the container customizations and the graph's add-node menu. Supports `BaseClass` / `BaseScriptStruct`, category meta filtering, alphabetical sorting (with `System` pinned), and per-session expansion state.
  - **`SScriptableNamePicker`** — typed name dropdown (locals / function pickers) with pin-type icons.
  - Built on top of **`FScriptableTypeCache`** — a smart cache that listens to Asset Registry events, Blueprint compilation, hot-reload, and class package load events so it always reflects the current set of available scriptable types (native + Blueprint + assets).

- **Sophisticated bindings UI**:
  - Per-property pill (`IN` / `OUT` / `CONTEXT`) with tooltips explaining the role.
  - Inline binding selector powered by Unreal's `IPropertyAccessEditor`, filtered by property compatibility (Object covariance, numeric/bool coercion, enum/byte, registered value converters), function purity, and `NoBinding` metadata anywhere in the chain.
  - Dedicated builders for arrays, structs, and dynamic property bags — including bags **nested inside** other structs (e.g. `FStateTreeReference::Parameters`).

- **Validation framework**:
  - A global `UScriptableFrameworkValidator` (Unreal `EditorValidatorBase`) walks any saved or validated asset, finds all nested `UScriptableObject`s, sanitizes obsolete bindings, and flags unbound Inputs and unresolvable Context properties.
  - The graph pair (`UScriptableGraphValidator` + `UScriptableBindingsValidator`) feeds the in-graph Validation panel with click-to-navigate issues and quick-fixes.

- **Context warnings** — when a wrapper task/condition references an asset that requires variables, a warning icon appears next to it if the parent scope doesn't satisfy the asset's expected context.

- **Module infrastructure** built on KzLib: `FScriptableFrameworkEditorModule` extends `FKzLibEditorModule_Base`, using its templated `RegisterAssetTypeAction` / `RegisterPropertyLayout` helpers for clean registration.

---

## Requirements

- **Unreal Engine 5.x** with C++20 enabled.
- A C++-enabled project (the plugin contains C++ source modules).
- **[KzLib](https://github.com/kirzo/KzLib)** plugin installed in the same project.
- A toolchain capable of compiling UE plugins:
  - **Windows** — Visual Studio 2022 with the *Game development with C++* workload.
  - **macOS** — Xcode (latest stable supported by your UE version).
  - **Linux** — Clang as configured by Epic for your engine version.
- Engine plugins used by the core (auto-listed in `ScriptableFramework.uplugin`): `PropertyAccessEditor`, `PropertyBindingUtils`, `DataValidation`.
- **Optional** plugins (each opt-in via its own `.uplugin`):
  - `ScriptableFrameworkAI` requires `StateTree` and `GameplayStateTree`.
  - `ScriptableFrameworkGAS` requires `GameplayAbilities`.
  - `ScriptableFrameworkSequencer` requires `LevelSequence` / `MovieScene` (engine modules, no extra plugin).
  - `ScriptableFrameworkUI` requires `UMG` (engine module, no extra plugin).

> ℹ️ ScriptableFramework's only third-party dependency is **KzLib**. Everything else is pure Unreal.

---

## Installation

ScriptableFramework is a family of standard Unreal Engine plugins. Pick whichever installation flow matches your workflow.

###  Project plugins (recommended)

```bash
cd <YourProject>/Plugins
git clone https://github.com/kirzo/KzLib.git
git clone https://github.com/kirzo/ScriptableFramework.git
```

Then:

1. Right-click your `.uproject` → **Generate Visual Studio project files**.
2. Open the project and build (or let the editor build it on first launch).
3. Enable the plugins you need via **Edit → Plugins**:
   - **KzLib** *(required)*
   - **Scriptable Framework** *(core, enabled by default)*
   - **Scriptable Framework - AI Integration** *(optional)*
   - **Scriptable Framework - GAS Integration** *(optional)*
   - **Scriptable Framework - Sequencer Integration** *(optional)*
   - **Scriptable Framework - UI** *(optional)*

### Option B — Git submodules

```bash
cd <YourProject>
git submodule add https://github.com/kirzo/KzLib.git Plugins/KzLib
git submodule add https://github.com/kirzo/ScriptableFramework.git Plugins/ScriptableFramework
git submodule update --init --recursive
```

### Using ScriptableFramework in your module

Add only the runtime modules you actually use to your `*.Build.cs`:

```csharp
PublicDependencyModuleNames.AddRange(new[] {
    "Core",
    "CoreUObject",
    "Engine",
    "GameplayTags",
    "KzLib",
    "ScriptableFramework"
    // "ScriptableFrameworkAI"        // optional — StateTree task
    // "ScriptableFrameworkGAS"       // optional — GAS tasks/conditions
    // "ScriptableFrameworkSequencer" // optional — Level Sequence tasks
    // "ScriptableFrameworkUI"        // optional — UMG tasks
});
```

---

## Repository Layout

The repo hosts **five sibling plugins**, one folder per plugin, each with its own `.uplugin`:

```
ScriptableFramework/                        # Repo root
├── ScriptableFramework/                    # Core plugin (Runtime + Converters + Uncooked + Editor)
│   ├── ScriptableFramework.uplugin
│   ├── Resources/                          # Plugin icon and editor sprites
│   └── Source/
│       ├── ScriptableFramework/            # Runtime module
│       │   ├── Public/
│       │   │   ├── Bindings/               # FScriptablePropertyBindings, ScriptableValueConverter
│       │   │   ├── ScriptableConditions/   # Condition (+ Compare, Logic, NestedRequirement), Requirement, RequirementAsset
│       │   │   ├── ScriptableTasks/        # Task (+ Debug, Flow, SetLocal, CallFunction, RunGraph, NestedAction), Action, ActionAsset, Async, Runner
│       │   │   ├── ScriptableNodes/        # Graph, GraphInstance, GraphSubsystem, Connection, Node + all node subclasses, AsyncRunScriptableGraph
│       │   │   ├── ScriptableBlueprintLibrary.h
│       │   │   ├── ScriptableContainer.h   # Base struct (Context, Locals, BindingSourceMap)
│       │   │   ├── ScriptableObject.h      # Base UObject (lifecycle, bindings, ID)
│       │   │   ├── ScriptableObjectAsset.h # Base asset (MenuCategory, Context schema)
│       │   │   ├── ScriptableRuntimeData.h # Injected Context/Locals/BindingsMap pointers
│       │   │   └── ScriptablePropertyUtilities.h
│       │   └── Private/                    # Implementation files (mirrors Public/)
│       │
│       ├── ScriptableFrameworkConverters/  # Runtime — value converters (e.g. KzTransformSourceConverter)
│       │
│       ├── ScriptableFrameworkUncooked/    # UncookedOnly — chainable Context-setter K2Nodes
│       │
│       └── ScriptableFrameworkEditor/      # Editor module
│           ├── Public/ScriptableFrameworkEd/
│           │   ├── Graph/                   # Ed-graph node base + registry, schema, pin, factories, commands
│           │   ├── Debug/                   # ScriptableDebugRegistry (live active-node halo)
│           │   └── Validation/              # Shared bindings validation pass
│           └── Private/
│               ├── Factories/               # Action / Requirement / Graph asset factories
│               └── ScriptableFrameworkEd/
│                   ├── Customization/       # Container / Object / Task / Condition / SetLocal / CallFunction (+ Widgets)
│                   ├── Graph/               # Editor toolkit, ed-nodes, schema, widgets, diff
│                   └── Validation/          # Framework / Graph / Bindings validators
│
├── ScriptableFrameworkAI/                  # AI integration plugin (StateTree)
│   └── Source/ScriptableFrameworkAI/Tasks/ # ScriptableTask_RunStateTree
│
├── ScriptableFrameworkGAS/                 # GAS integration plugin
│   └── Source/ScriptableFrameworkGAS/
│       ├── Tasks/                          # GrantAbility, ActivateAbilityByTag, ManageGameplayTag, SendGameplayEvent
│       └── Conditions/                     # HasAbility, CanActivateByTag, MatchTagQuery
│
├── ScriptableFrameworkSequencer/           # Level Sequence integration plugin
│   └── Source/ScriptableFrameworkSequencer/Tasks/  # PlayLevelSequence, WaitForLevelSequenceFinish, SetLevelSequencePlaybackPosition
│
├── ScriptableFrameworkUI/                  # UMG integration plugin
│   └── Source/ScriptableFrameworkUI/Tasks/ # CreateWidget, RemoveWidget
│
├── LICENSE                                 # MIT
└── README.md
```

---

## Usage Examples

### Define and run an Action in C++

```cpp
#include "ScriptableTasks/ScriptableAction.h"
#include "ScriptableTasks/ScriptableTask_Flow.h"
#include "ScriptableTasks/ScriptableTask_Debug.h"

FScriptableAction Action;
Action.Mode = EScriptableActionMode::Sequence;

// Wait 1.5s then log
auto* Wait = NewObject<UScriptableTask_Wait>(MyOwner);
Wait->Duration = 1.5f;
Action.Tasks.Add(Wait);

auto* Log = NewObject<UScriptableTask_LogMessage>(MyOwner);
Log->Message = TEXT("Hello after waiting!");
Action.Tasks.Add(Log);

Action.OnActionFinish.AddLambda([]
{
    UE_LOG(LogTemp, Log, TEXT("Done!"));
});

Action.Run(MyOwner);
```

### Fire-and-forget (no member storage)

```cpp
FScriptableAction TempAction;
TempAction.Mode = EScriptableActionMode::Sequence;
// ... populate Tasks ...

// Action is moved into an internal runner that keeps it alive until OnActionFinish.
FScriptableAction::Run(MoveTemp(TempAction), this);
```

### Run a Scriptable Action asynchronously from Blueprints

<p align="center">
    <img src="https://kirzo.dev/content/images/plugins/ScriptableFramework/bp_run_action.jpg" alt="Run Scriptable Action from Blueprint" width="768">
</p>

Backed by `UAsyncRunScriptableAction`. The node auto-resets the action when it finishes, so the same action struct is safe to reuse.

### Run a Scriptable Graph from Blueprints

<p align="center">
    <img src="https://kirzo.dev/content/images/plugins/ScriptableFramework/bp_run_graph.jpg" alt="Run a Scriptable Graph from Blueprint" width="768">
</p>

Backed by `UAsyncRunScriptableGraph`. `Started` hands you the live runner (call `Fire Event` or `Set Graph Instance Context Property` on it while it runs); `Finished` fires when the graph completes or is cancelled. Cancel any time via the Scriptable Graph subsystem (`Cancel Runner`, `Cancel Runners By Id`, …).

### Evaluate a Requirement

```cpp
#include "ScriptableConditions/ScriptableRequirement.h"

if (FScriptableRequirement::EvaluateRequirement(this, MyRequirement))
{
    // All conditions passed (or any, depending on Mode + Negate)
}
```

### Set a Context parameter from Blueprint

<p align="center">
    <img src="https://kirzo.dev/content/images/plugins/ScriptableFramework/bp_req_eval.jpg" alt="Set Context parameter and evaluate requirement from Blueprint" width="768">
</p>  

### Implement a custom Condition with bindings

```cpp
UCLASS(DisplayName = "My Custom Check", meta = (ConditionCategory = "Game|Custom"))
class UMyCondition : public UScriptableCondition
{
    GENERATED_BODY()

public:
    /** Bindable property — auto-wired to a 'Health' on the parent context if present. */
    UPROPERTY(EditAnywhere, Category = "Config", meta = (ScriptableContext))
    float Health = 0.f;

    UPROPERTY(EditAnywhere, Category = "Config")
    float Threshold = 50.f;

protected:
    virtual bool Evaluate_Implementation() const override { return Health < Threshold; }

#if WITH_EDITOR
    virtual FText GetDisplayTitle() const override
    {
        FString Bound;
        FText Lhs = GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UMyCondition, Health), Bound)
                    ? FText::FromString(Bound)
                    : FText::AsNumber(Health);
        return FText::Format(INVTEXT("{0} < {1}"), Lhs, FText::AsNumber(Threshold));
    }
#endif
};
```

### Play a Level Sequence and wait for it to finish

<p align="center">
    <img src="https://kirzo.dev/content/images/plugins/ScriptableFramework/sequence_player.jpg" alt="Play a Level Sequence and wait for it to finish" width="768">
</p>  

Here the tasks "Set Level Sequence Playback Position" and "Play Level Sequence" are auto-binded to the Output property "Player" from "Create Level Sequence Player".

## Related Projects

- **[KzLib](https://github.com/kirzo/KzLib)** — the foundation utility library used by ScriptableFramework (math, geometry, spatial, ECS, editor toolkit). **Required dependency**.
- **[Axon Physics](https://kirzo.dev/axon-physics/)** — custom simulation/physics layer also built on KzLib.

---

## Contributing

Contributions are welcome! If you'd like to help:

1. Fork the repository.
2. Create a feature branch: `git checkout -b feature/my-feature`.
3. Commit with clear messages and follow the existing code style (Unreal/Epic C++ conventions).
4. Open a Pull Request describing **what** changed and **why**.

For larger changes please open an issue first so we can align on direction before you invest time in the implementation.

---

## License

ScriptableFramework is released under the **MIT License**. See [LICENSE](LICENSE) for the full text.

You are free to use it in commercial and non-commercial projects.

---

## Author

Built and maintained by **[Kirzo](https://kirzo.dev/)**.

- 🌐 Website: [kirzo.dev](https://kirzo.dev/)
- 🐙 GitHub: [@kirzo](https://github.com/kirzo)

If ScriptableFramework helps your project, consider giving the repository a ⭐ — it really helps visibility.

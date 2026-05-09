<p align="center">
  <img src="https://kirzo.dev/content/images/plugins/ScriptableFramework_banner.jpg" alt="ScriptableFramework Banner" width="512">
</p>

<h1 align="center">ScriptableFramework</h1>

<p align="center">
  <em>A data-driven gameplay framework for Unreal Engine — composable Tasks &amp; Conditions, automatic property bindings, reusable assets, and a polished node-graph-like editor experience.</em>
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
  - [Context System](#context-system)
  - [Property Bindings](#property-bindings)
  - [Reusable Assets](#reusable-assets)
  - [Async &amp; Fire-and-Forget Execution](#async--fire-and-forget-execution)
  - [Built-in Tasks](#built-in-tasks)
  - [Built-in Conditions](#built-in-conditions)
  - [StateTree Integration](#statetree-integration)
  - [Gameplay Ability System Integration](#gameplay-ability-system-integration)
  - [Level Sequence Integration](#level-sequence-integration)
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

**ScriptableFramework** is an open-source gameplay plugin suite for **Unreal Engine 5** that lets designers and programmers build complex behaviour from small, composable, data-driven blocks: **Tasks**, **Actions**, **Conditions** and **Requirements**. It comes with a sophisticated **property bindings system** (Context, Sibling, Function, Auto-bindings), reusable **Asset workflows** (Action assets, Requirement assets), **wrapper nodes** to embed assets inline, **deep editor integration** (custom pickers, validators, drag-and-drop UI, copy/paste), and out-of-the-box integrations with **StateTree**, **Gameplay Abilities**, and **Level Sequences**.

It is built on top of **[KzLib](https://github.com/kirzo/KzLib)** — leveraging its `FKzParamDef`, `FInstancedPropertyBag` helpers, asset-editor toolkit and Slate widgets — so you get a battle-tested foundation without reinventing the wheel.

The framework is **modular**, **Blueprint-friendly**, and designed to scale from simple gameplay scripts (e.g. *"wait 2 seconds, then spawn a particle"*) to complex hierarchical behaviour graphs with shared parameters and runtime-driven logic.

---

## Plugins & Modules

ScriptableFramework ships as a **family of independent Unreal plugins** so you can pick exactly what your project needs. Each integration sits in its own `.uplugin` and depends on the core plugin being enabled.

| Plugin | Modules | Enabled by default | Purpose |
| :--- | :--- | :---: | :--- |
| **`ScriptableFramework`** | `ScriptableFramework` (Runtime), `ScriptableFrameworkEditor` (Editor) | ✅ | Core: ScriptableObject, Tasks, Actions, Conditions, Requirements, Bindings, Context, Async, Editor toolkit. |
| **`ScriptableFrameworkAI`** | `ScriptableFrameworkAI` (Runtime) | 🔲 | StateTree integration — `Run StateTree` task with parameter binding support. |
| **`ScriptableFrameworkGAS`** | `ScriptableFrameworkGAS` (Runtime) | 🔲 | Gameplay Ability System — granting abilities, sending events, tag management. |
| **`ScriptableFrameworkSequencer`** | `ScriptableFrameworkSequencer` (Runtime) | 🔲 | Level Sequence — play sequences, wait for finish, set playback position. |

The runtime modules only depend on Unreal Engine and KzLib. The editor module loads in editor builds only and never enters cooked builds. Optional plugins are disabled by default; enable only the ones you need.

---

## Feature Tour

### Tasks &amp; Actions

**`UScriptableTask`** is the unit of execution. Tasks have a status (`None` / `Begun` / `Finished`), latent execution, and four overridable hooks: `BeginTask`, `FinishTask`, `ResetTask`, plus the implicit `Tick`. Both C++ and Blueprint subclasses are first-class:

- **Control settings (`FScriptableTaskControl`)**:
  - `bLoop` + `LoopCount` (0 = infinite) — task automatically restarts on `Finish()`.
  - `bDoOnce` — task only ever runs once during its lifetime.
- **Native &amp; Dynamic delegates** for `OnTaskBegin` / `OnTaskFinish`, both BP-assignable and C++-bindable.

**`FScriptableAction`** is a container struct holding a list of tasks, an execution `Mode` (Sequence or Parallel), and a shared **Context**. Actions are the entry point most users hit:

- **`Sequence`** — tasks run one after another. The action finishes when the last one finishes.
- **`Parallel`** — all tasks start simultaneously. The action finishes when every task has finished.
- **`Run(Owner)`** registers all sub-tasks, injects the binding map and context, and kicks off execution.
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
- **Nested groups**: `UScriptableCondition_Group` lets you embed a full `FScriptableRequirement` inside another requirement, with its own local context — perfect for `(A AND B) OR (C AND D)` expressions in the UI.

<p align="center">
    <img src="https://kirzo.dev/content/images/plugins/ScriptableFramework/ScriptableFrameworkConditions.gif" width="768">
</p>

### Context System

Every container (`FScriptableAction`, `FScriptableRequirement`) and asset can declare a **Context** — a typed parameter bag shared with all child nodes:

- **`ContextDefinitions`** — a `TSet<FKzParamDef>` declaring the schema (name + type for each parameter). Powered by KzLib's `FKzParamDef`, so you get the full editor pin-type picker with arrays, structs, enums, soft references, etc.
- **`Context` (`FInstancedPropertyBag`)** — the runtime memory backing the schema. Built automatically by `ConstructContext()`.
- **Templated accessors**: `AddContextProperty<T>`, `SetContextProperty<T>`, `GetContextProperty<T>` (KzLib-backed) for type-safe C++ usage.
- **Blueprint setters with custom thunks**: `SetActionContextParameter` / `SetRequirementContextParameter` are wildcard-pin nodes that accept any value type and runtime-validate it against the schema (with safe failure and `KismetExecutionMessage` warnings on type mismatch).
- **Inheritance**: containers automatically fall back to the parent scope's context if their own is empty, enabling hierarchical scoping.

### Property Bindings

This is where ScriptableFramework really shines. The bindings system lets any property on a Task or Condition pull its value from somewhere else at runtime:

- **`FScriptablePropertyBindings`** — list of `FScriptablePropertyBinding` entries (each with a Source path, Target path, Source Guid, and `bIsAutoBinding` flag).
- **Path-based resolution** — `FPropertyBindingPath` describes traversal segments (property name + optional array index). The custom `ResolveIndirections` walks the path manually so it can dive into:
  - Standard struct members and array elements.
  - **`FInstancedPropertyBag` payloads** anywhere in the chain (e.g. binding to a dynamic StateTree parameter or a Context variable).
  - **UFunctions mid-path** — pure / const Blueprint-callable functions that take no parameters and return a value (e.g. `GetActorLocation().X`).
- **Source types**:
  - **Context** binding (no source GUID) — read from the parent container's Context bag.
  - **Sibling** binding (source GUID) — read from a previous task or condition in the same parent, found via the `BindingSourceMap`.
- **Type coercion** at resolve time: identical types use a fast `CopyCompleteValue`; the system also handles `UObject` ↔ `TObjectPtr`, child-class to parent-class casts, `Object → Bool` (validity check), `Numeric ↔ Numeric`, `Bool ↔ Numeric`, and `Byte ↔ Enum`.
- **Auto-bindings** — properties marked with metadata `meta = (ScriptableContext)` (or `Category = "Context"`) are automatically wired to a compatible variable in scope. Re-baked on every save and editor change so renames / type changes are caught immediately.
- **Property categorization metadata** — `ScriptableInput`, `ScriptableOutput`, `ScriptableContext` (or matching `Category` prefixes) drive editor pills (`IN` / `OUT` / `CONTEXT`) and validation rules.
- **Sanitization** — `SanitizeObsoleteBindings` removes broken bindings on load; array element removal/clear automatically shifts indices in any binding pointing into that array.

<p align="center">
    <img src="https://kirzo.dev/content/images/plugins/ScriptableFramework/requirement_2.gif" alt="Binding a property to a Context variable via the inline picker" width="768">
</p>

### Reusable Assets

Both Actions and Requirements have first-class asset variants for reuse and modularity:

- **`UScriptableActionAsset`** — a `Const` data asset wrapping an `FScriptableAction`. Supports a `MenuCategory` (e.g. `Combat|Melee`) that powers the editor picker organization.
- **`UScriptableRequirementAsset`** — same idea for requirements.
- **Wrapper nodes** make assets usable inline in any list:
  - **`UScriptableTask_RunAsset`** — runs an `UScriptableActionAsset` as a sub-task. On `OnRegister` it deep-copies the asset's tasks into a transient runtime instance and inherits the parent context, so the asset's parameters can be filled from outside.
  - **`UScriptableCondition_Asset`** — same idea for requirements: builds a transient `UScriptableCondition_Group` with cloned conditions and inherited context, then evaluates it like any other condition.
- **Context propagation**: the wrapper passes its parent context down to the embedded asset, so an asset declaring a `Target` parameter automatically gets fed by the outer scope.
- **Asset Registry tags**: `MenuCategory` is exposed as an alphabetical asset registry tag so the type picker can surface it instantly.

### Async &amp; Fire-and-Forget Execution

ScriptableFramework offers two convenient ways to run an Action without manually managing its lifetime:

- **`UAsyncRunScriptableAction`** — a `UBlueprintAsyncActionBase` node that gives Blueprints a clean async/await-style API. It registers with the game instance, hooks into `OnActionFinish`, broadcasts the `OnFinish` BP delegate, and auto-cleans the action via `Reset()` on completion.

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

Both expose dynamic display titles so the bound parameters show up directly in the editor list (`Wait "Duration" s`, `Log: "MyMessage"`, …).

### Built-in Conditions

A complete logic toolkit out of the box:

- **`UScriptableCondition_Bool`** — checks a single bool. Display title respects bindings and negation: `IsAlive`, `!IsAlive`, `Is True`, `Is False`.
- **`UScriptableCondition_CompareNumbers`** — typed comparison (`==`, `!=`, `<`, `<=`, `>`, `>=`) with configurable `ErrorTolerance` for float equality.
- **`UScriptableCondition_CompareBooleans`** — `AND`, `OR`, `XOR`, `NAND`, `==`, `!=`.
- **`UScriptableCondition_Distance`** — squared-distance check between two actors with the same set of comparison operators.
- **`UScriptableCondition_Probability`** — `0..1` chance, useful for non-deterministic gameplay (`30% Chance`).
- **`UScriptableCondition_IsValid`** — null/garbage-collection check on any UObject reference.
- **`UScriptableCondition_Group`** — nested requirement with its own context, mode and negation. Renders inline like a fold-out group in the editor.

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

Combined with the rest of the framework, you can sequence cinematics with gameplay logic naturally: *grant ability → wait sequence → send event → run another asset*, all in one Action.

### Editor Tooling

`ScriptableFrameworkEditor` is a sizeable editor toolkit, not just a couple of customizations:

- **Asset workflow**:
  - Asset factories for `UScriptableActionAsset` and `UScriptableRequirementAsset` under the `ScriptableFramework` Content Browser category.
  - Custom thumbnails, icons and category colors registered via `FScriptableFrameworkEditorStyle` (KzLib's `TKzEditorStyle_Base` CRTP).

- **Property customizations** — every container, task, condition and group has its own `IPropertyTypeCustomization`:
  - **`FScriptableContainerCustomization`** (base) — header with title, mode pill (`AND`/`OR`, `Sequence`/`Parallel`), Add button (with full type picker), Remove-all button, Context editor button (modal `IStructureDetailsView` for `ContextDefinitions`).
  - **`FScriptableObjectCustomization`** (base) — header with checkbox, dynamic title, type picker, Use-Selected/Browse/Edit buttons, options menu (Replace With, Copy, Paste, Duplicate, Remove, Clear).
  - **`FScriptableTaskCustomization`** — adds `Once` and `Loop` toggle pills with inline `LoopCount` editor.
  - **`FScriptableConditionCustomization`** — adds `NOT` toggle pill.
  - **`FScriptableConditionGroupCustomization`** — merges the Group object with its inner Requirement (renamable group name, mode toggle, negate, etc.).
  - **`FScriptableRequirementCustomization` / `FScriptableActionCustomization`** — the concrete container customizations.

- **Reusable Slate widgets**:
  - **`SScriptableTypePicker`** — combo button with searchable, hierarchical type picker. Supports `BaseClass` / `BaseScriptStruct`, category meta filtering, alphabetical sorting (with the `System` category always pinned), expansion-state persistence per session.
  - **`SScriptableTypeSelector`** — the underlying tree-view widget, reusable in custom menus.
  - Built on top of **`FScriptableTypeCache`** — a smart cache that listens to Asset Registry events, Blueprint compilation, hot-reload, and class package load events so it always reflects the current set of available scriptable types (native + Blueprint + assets).

- **Sophisticated bindings UI**:
  - Per-property pill (`IN` / `OUT` / `CONTEXT`) with tooltips explaining the role.
  - Inline binding selector powered by Unreal's `IPropertyAccessEditor`, filtered by:
    - `ArePropertiesCompatible` (handles Object covariance, numeric/bool coercion, enum/byte, etc).
    - Function purity (only `BlueprintPure` / `Const`, zero parameters, single return value).
    - `NoBinding` metadata anywhere in the chain.
  - Cached display state with auto-invalidation on global property change events.
  - Dedicated builders for arrays (`FScriptableArrayBuilder`), structs (`FScriptableStructBuilder`), and dynamic property bags (`FPropertyBagBindingsBuilder`) — including detection of bags **nested inside** other structs (e.g. `FStateTreeReference::Parameters`).

- **Validation framework**:
  - `UScriptableFrameworkValidator` — automatic data validation via Unreal's `EditorValidatorBase`. Walks any saved or validated asset, finds all nested `UScriptableObject`s, runs `SanitizeObsoleteBindings`, and flags:
    - **Inputs without manual bindings** (`Input property MUST be connected`).
    - **Context properties** that can't be auto-resolved and lack a manual override.

- **Context warnings** — when a wrapper task/condition references an asset that requires variables, a warning icon appears next to the wrapper if the parent scope doesn't satisfy the asset's expected context.

- **Module infrastructure** built on KzLib:
  - `FScriptableFrameworkEditorModule` extends `FKzLibEditorModule_Base`, using its templated `RegisterAssetTypeAction` / `RegisterPropertyLayout` helpers — clean and concise registration code.

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
});
```

---

## Repository Layout

The repo hosts **four sibling plugins**, one folder per plugin, each with its own `.uplugin`:

```
ScriptableFramework/                        # Repo root
├── ScriptableFramework/                    # Core plugin (Runtime + Editor)
│   ├── ScriptableFramework.uplugin
│   ├── Resources/                          # Plugin icon and editor sprites
│   └── Source/
│       ├── ScriptableFramework/            # Runtime module
│       │   ├── Public/
│       │   │   ├── Bindings/               # FScriptablePropertyBindings (paths, resolution, sanitization)
│       │   │   ├── ScriptableConditions/   # ScriptableCondition (+ Compare, Logic, Group), ScriptableRequirement, RequirementAsset
│       │   │   ├── ScriptableTasks/        # ScriptableTask (+ Debug, Flow), ScriptableAction, ActionAsset, AsyncRunScriptableAction
│       │   │   ├── ScriptableBlueprintLibrary.h
│       │   │   ├── ScriptableContainer.h   # Base struct (Context, BindingSourceMap)
│       │   │   ├── ScriptableObject.h      # Base UObject (lifecycle, tick, bindings, ID)
│       │   │   ├── ScriptableObjectAsset.h # Base asset (MenuCategory, Context schema)
│       │   │   ├── ScriptableObjectTypes.h # FScriptableObjectTickFunction
│       │   │   └── ScriptablePropertyUtilities.h
│       │   └── Private/                    # Implementation files (mirrors Public/)
│       │       └── ScriptableTasks/
│       │           └── ScriptableActionRunner.{h,cpp}  # Internal runner for fire-and-forget Run()
│       └── ScriptableFrameworkEditor/      # Editor module
│           ├── Public/
│           │   ├── ScriptableFrameworkEditor.h
│           │   ├── ScriptableFrameworkEditorHelpers.h
│           │   ├── ScriptableFrameworkEditorStyle.h
│           │   ├── ScriptableSchema.h
│           │   └── ScriptableTypeCache.h
│           └── Private/
│               ├── Factories/              # ScriptableConditionAssetFactory, ScriptableTaskAssetFactory
│               └── ScriptableFrameworkEd/
│                   ├── Customization/      # Action / Container / Object / Task / Condition / ConditionGroup / Requirement
│                   │   └── Widgets/        # SScriptableTypePicker, SScriptableTypeSelector
│                   └── Validation/         # ScriptableFrameworkValidator
│
├── ScriptableFrameworkAI/                  # AI integration plugin (StateTree)
│   ├── ScriptableFrameworkAI.uplugin
│   └── Source/ScriptableFrameworkAI/
│       ├── Public/Tasks/ScriptableTask_RunStateTree.h
│       └── Private/Tasks/ScriptableTask_RunStateTree.cpp
│
├── ScriptableFrameworkGAS/                 # GAS integration plugin
│   ├── ScriptableFrameworkGAS.uplugin
│   └── Source/ScriptableFrameworkGAS/
│       ├── Public/
│       │   ├── Tasks/                      # GrantAbility, ActivateAbilityByTag, ManageGameplayTag, SendGameplayEvent
│       │   └── Conditions/                 # HasAbility, CanActivateByTag, MatchTagQuery
│       └── Private/
│
├── ScriptableFrameworkSequencer/           # Level Sequence integration plugin
│   ├── ScriptableFrameworkSequencer.uplugin
│   └── Source/ScriptableFrameworkSequencer/
│       ├── Public/Tasks/                   # PlayLevelSequence, WaitForLevelSequenceFinish, SetLevelSequencePlaybackPosition
│       └── Private/
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
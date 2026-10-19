---
name: generator
description: This skill should be used when the user provides an image of a statechart, UML state machine or state diagram and asks to implement, generate or convert it to C++, e.g. "implement this state diagram in C++", "convert this statechart to Boost.MSM", "generate Boost MSM code for this FSM", "turn this hierarchical state machine into an active object with GoogleTest tests and CMake". Not for other state machine libraries (Boost.SML, Qt SCXML), non-C++ targets, or diagrams given only as text or PlantUML.
---

# Generating Boost.MSM C++ Code from State Diagrams

## Overview

Turn an image of a state diagram into a Boost.MSM state machine, an active object around it, GoogleTest unit tests and a CMake project that builds without warnings. The input must be an image (e.g., PNG or JPEG); if the user gives the diagram only as text or PlantUML, ask for an image.

## Requirements

Steps 2 and 5 run on the user's machine and need `python3`, `cmake` 3.16 or later, a C++14 compiler, the Boost headers (1.70 or later) and `clang-format`. Before Step 2, check them (`python3 --version`, `cmake --version`, `c++ --version`, `clang-format --version`, and look for `boost/msm/` under the compiler's include paths, e.g. `/usr/include/boost/msm/`).

* If `python3` is missing, the project cannot be scaffolded: tell the user what to install and stop after Step 1.
* If only the build tools or Boost are missing, complete Steps 1 to 4, then tell the user which tools are missing and that Step 5 must run on a machine that has them.

---

## The Workflow

### Step 1: Generate & Verify the UML Topology Table (Mandatory First Step)

Before writing any code, analyze the input statechart or state diagram image and translate its structure and behavior into a structured markdown table. The table must conform strictly to the following 9-column schema:

| States / Pseudo-state Names | Type | Ancestor | Entry Actions | Exit Actions | Transition Trigger | Guard | Action Effect | Next State |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |

> Show the table to the user, then **stop and wait** for them to confirm or correct it. Do not start Step 2 until the user confirms: the code is built from this table, and a misread diagram is cheapest to fix here. Only if the user's request already says to proceed without confirmation (e.g., a non-interactive run), say so and continue.

When you show the table, also save it to `<StateMachineName>/<StateMachineName>.md`, where `<StateMachineName>` is the CamelCase name later passed to the scaffolding script and the directory is the one it will create (under `--output-dir`, if given; e.g., `SelectiveStart/SelectiveStart.md`). Use this layout:

```markdown
# <StateMachineName> Topology Table

Source diagram: `<diagram file name>`

<the table, exactly as shown to the user>
```

Writing it before Step 2 is safe: the scaffolding script never creates or overwrites this file, even with `--force`. If the user corrects the table, update the file so it always matches the table the implementation is built from.

#### Schema Formatting Rules

* **States / Pseudo-state Names**: Name of the state, substate, or pseudo-state (e.g., `[*]`, `S1`, `Idle`).
* **Type**: Type of state (e.g., `Initial Pseudo-state`, `Simple State`, `Composite State`, `Choice Pseudo-state`, `History Pseudo-state`, `Entry Point`, `Exit Point`, `Final State`).
* **Ancestor**: The parent state encapsulating this state. If the state is at the top-level (direct child of the main statechart), use a dash `—`.
* **Entry Actions**: Use `None` if empty.
* **Exit Actions**: Use `None` if empty.
* **Transition Trigger**: The event that triggers a transition, written as the CamelCase name it gets in code (`Event::<Name>`), e.g. diagram `evStart` → `EvStart`, `start()` → `Start`. Add its parameters in parentheses when the diagram shows them (e.g., `EventB(name, id)`). Use `None (Anonymous)` for event-less transitions, and a dash `—` if no outgoing transitions exist.
* **Guard**: Use `None` if empty. For the `[else]` branch of a choice, write `else`.
* **Action Effect**: Use `None` if empty. On a deferred event's row, write the keyword `Defer`, which is the action of its Boost.MSM row.
* **Next State**: The destination state of the transition. Use a dash `—` if no outgoing transitions are defined. Write the bare state name with no annotations, except for these two keywords:
  * `Internal`: an internal transition, which runs its effect without exiting or re-entering the state.
  * `Deferred`: the state defers the event in Transition Trigger. Its Action Effect is always `Defer`.

  A transition from outside a composite state into one of its substates names the substate itself (e.g., `S41`, not `S4`); the substate's Ancestor column already shows that it is an entry into the composite. A self-transition, which exits and re-enters the state and so runs its exit and entry actions, names the state itself; use `Internal` only when the state is not exited.

  A history pseudo-state gets its own row, named `H` (`H1`, `H2`, ... when the diagram has several), with Type `History Pseudo-state` and its composite as Ancestor. A transition into the history names it as Next State (e.g., `H`). If the history has a default transition, the `H` row has Transition Trigger `None (Anonymous)` and that transition's target as Next State; otherwise it has `—` for both.

Write one row per outgoing transition, and repeat the state's Name, Type, Ancestor, Entry Actions and Exit Actions on every row of that state. A state without outgoing transitions gets exactly one row, with `—` as Transition Trigger and Next State. Every `—` in the table is an em dash (U+2014), not a hyphen.

#### Example Table

A top-level state `Idle` with a deferred event, an internal transition and a direct entry into a substate; a choice with an `else` branch; and a composite `Busy` with a self-transition:

| States / Pseudo-state Names | Type | Ancestor | Entry Actions | Exit Actions | Transition Trigger | Guard | Action Effect | Next State |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `[*]` | Initial Pseudo-state | — | None | None | None (Anonymous) | None | None | Idle |
| Idle | Simple State | — | None | None | Start | None | None | Check |
| Idle | Simple State | — | None | None | Tick | None | Defer | Deferred |
| Idle | Simple State | — | None | None | Ping | None | reply() | Internal |
| Idle | Simple State | — | None | None | Boost | None | None | Fast |
| Check | Choice Pseudo-state | — | None | None | None (Anonymous) | isReady() | None | Busy |
| Check | Choice Pseudo-state | — | None | None | None (Anonymous) | else | None | Idle |
| Busy | Composite State | — | startMotor() | stopMotor() | Stop | None | None | Idle |
| Busy | Composite State | — | startMotor() | stopMotor() | Reset | None | None | Busy |
| `[*]` | Initial Pseudo-state | Busy | None | None | None (Anonymous) | None | None | Slow |
| Slow | Simple State | Busy | None | None | Tick | None | None | Fast |
| Fast | Simple State | Busy | None | None | — | None | None | — |

#### Reading the Diagram

Identify each element's Type by its UML shape, not by its name or position:

| Shape | Type |
| :--- | :--- |
| Filled black circle | `Initial Pseudo-state` |
| Rounded rectangle | `Simple State`, or `Composite State` when it contains states |
| Diamond | `Choice Pseudo-state` |
| Circle with `H` | `History Pseudo-state` (shallow) |
| Circle with `H*` | Deep history: not supported by Boost.MSM (see below) |
| Circle with an `X` on a composite's border | `Exit Point` |
| Hollow circle on a composite's border | `Entry Point` |
| Filled circle inside a ring (bull's-eye) | `Final State` |

Elements drawn in red and struck through are the original UML intent that Boost.MSM cannot express directly; the black elements next to them are the intended implementation. Leave the red elements out of the table.

Boost.MSM has no deep history (`H*`): its history policies are shallow, so they restore only the last active substate of one composite, and a restored nested composite restarts at its initial state. If the diagram has a deep history, tell the user in Step 1 that Boost.MSM does not support it, and ask whether to implement it as a shallow history (`H`) instead. Do not continue until the user decides.

---

### Step 2: Scaffold the Project

Do not create the project files by hand. Run the bundled scaffolding script, `scripts/scaffold_statemachine.py` in this skill's directory:

```bash
python3 ${CLAUDE_SKILL_DIR}/scripts/scaffold_statemachine.py <StateMachineName> \
    --events <Event1>,<Event2>,... [--output-dir <dir>] [--force]
```

* `<StateMachineName>`: CamelCase name of the state machine (e.g., `SelectiveStart`).
* `--events`: every distinct Transition Trigger from the table except `None (Anonymous)`, CamelCase, at most 50. Omit it for a state machine without events.
* `--output-dir`: directory in which `<StateMachineName>/` is created (default: current directory).
* `--force`: overwrite an existing scaffold. Without it the script refuses (exit status 2) to touch existing files.

It generates a project that builds, runs and passes its tests before any implementation:

```
<StateMachineName>/
├── .clang-format                      code style; clang-format picks it up from any file below
├── <StateMachineName>.md              topology table from Step 1 (written by you, not the script)
├── CMakeLists.txt                     Boost, -Wall -Wextra -Werror, MPL limits raised to 50
├── src/
│   ├── <StateMachineName>.hpp         state machine skeleton: placeholder Idle state, empty table
│   ├── <StateMachineName>.cpp         front-end constructor (logger injection)
│   ├── <StateMachineName>ActObj.hpp   active object: post(), dispatch(), isFlagActive(), waitForFlag()
│   ├── <StateMachineName>ActObj.cpp
│   ├── Event.hpp                      one empty struct per --events entry, in namespace Event
│   ├── EventQueue.hpp                 bounded thread-safe queue of the events variant
│   ├── Log.hpp / Log.cpp              injectable logger interface and console logger
│   └── main.cpp                       interactive app: one key per event, h = help, q = quit
└── tests/
    ├── CMakeLists.txt                 GoogleTest (installed, else fetched)
    └── test_<StateMachineName>.cpp    placeholder tests and a RecordingLog helper
```

---

### Step 3: Consult Reference Examples

Before writing the implementation, consult the examples listed in [examples.md](examples.md) that match the features identified in the table from Step 1 (e.g., choice pseudo-states, hierarchy, deferred events, history, initial-transition actions).

---

### Step 4: Implement the State Machine (`src/<StateMachineName>.hpp`)

Replace every `TODO` of the skeleton with the content of the topology table:

1. Replace the placeholder `Idle` state with one `msm::front::state<>` per simple state. Implement each Entry/Exit Action in `on_entry`/`on_exit`. Trace through `fsm.m_logger`, so tests can assert on the trace order.
2. Add one functor per Action Effect and one `bool` functor per Guard.
   Write every `on_entry`/`on_exit` and every functor out in full, even when several look alike. Do not generate them with preprocessor macros (e.g. `TRACE_STATE(S1)`): macros hide the code from the reader, the debugger and clangd.
3. Turn the table rows into transition table rows. The skeleton defines `Row`, `none` and `Defer` shorthands, visible in nested composites too.
   * Add one `Row<Source, Event::Trigger, Target, Action, Guard>` per table row that has a transition, using `none` for empty cells. Skip rows whose Transition Trigger is `—`. Initial Pseudo-state rows become `initial_state`, not a Row.
   * Put each Row in the transition table of the (sub)machine named in its source state's Ancestor column; for `—`, the top-level table.
   * A Row can only target states of its own machine. When Next State has another Ancestor than the source: entering a composite targets `S::direct<S_::Sub>`; leaving one needs an Exit Point (`exit_pseudo_state` in the composite, and an `S::exit_pt<S_::X>` row in the parent).
   * A row whose Next State is a history pseudo-state targets the history's composite (e.g., `Row<Paused, Event::Resume, Operation, none, none>`). The `H` row itself is not a Row; it becomes the composite's history policy (see History Pseudo-state below).
   * A deferred row (Action Effect `Defer`, Next State `Deferred`) becomes `Row<S, Event::Ev, none, Defer, none>`, and the (sub)machine whose table holds it needs `typedef int activate_deferred_events;`. MSM replays a deferred event after the next transition.

   Start every transition table with the column header below, and align each row to its columns (keep the table between `// clang-format off` and `// clang-format on`):

   ```cpp
   struct transition_table : mpl::vector<
       //    Start     Event         Next      Action               Guard
       //  +---------+--------------+---------+---------------------+----------------------+
       Row < Idle    , Event::Start , Running , startProcess        , none                 >,
       Row < Running , Event::Stop  , Idle    , none                , none                 >
   > {};
   ```
4. Add a flag (`struct XFlag {}` plus `typedef mpl::vector1<XFlag> flag_list;`) to every stable state (not to pseudo-states), so the tests can check the state each row reaches.
5. Add any event data that guards or actions read to `src/Event.hpp`.

Map each table Type as follows:

| Table Type | Boost.MSM construct |
| :--- | :--- |
| Simple State | `struct S : msm::front::state<>` |
| Composite State | Nested `struct S_ : msm::front::state_machine_def<S_>` plus `typedef msm::back::state_machine<S_> S;` |
| Initial Pseudo-state | `typedef S initial_state;` in the owning (sub)machine. An initial transition with an effect needs a transient state with an anonymous transition (see `Mgr` in examples.md). |
| Choice Pseudo-state | A plain state with anonymous rows (`Event` = `none`), one per branch, each with its guard. Guards must be mutually exclusive; implement an `else` guard as the negation of the other branches' guards. |
| History Pseudo-state | A back-end policy of its composite, chosen by how the composite is entered. If the composite's Initial Pseudo-state targets `H`, every entry uses the history: `msm::back::state_machine<S_, msm::back::AlwaysHistory>`, with the `H` row's default target as `initial_state` (see `Interrupt`). Otherwise only the transitions into `H` use it: `msm::back::state_machine<S_, msm::back::ShallowHistory<mpl::vector<Event::Ev1, ...>>>`, listing the Transition Trigger of every row whose Next State is `H`, while a transition into the composite itself starts at `initial_state` (see `Urgent`). |
| Final State | `msm::front::terminate_state<>` at the top level. Inside a composite, use an `exit_pseudo_state<Ev>` and `S::exit_pt<...>` row (see `SimpleState`). |
| Exit Point | `struct X : msm::front::exit_pseudo_state<Ev>` in the composite, plus a parent row from `S::exit_pt<S_::X>` (see `SimpleState`) |
| Entry Point / transition into a substate | Target substate also inherits `msm::front::explicit_entry<0>`; the row targets `S::direct<S_::Sub>` (see `SimpleState`) |
| Orthogonal regions | `typedef mpl::vector<R1Initial, R2Initial> initial_state;` |
| Internal transition (`Internal`) | Row with `none` as Target |
| Deferred event (`Defer` / `Deferred`) | `Row<S, Event::Ev, none, Defer, none>`, plus `typedef int activate_deferred_events;` in the (sub)machine (see `SimpleState`) |

Then rewrite `tests/test_<StateMachineName>.cpp`. Write at least one test per table row (target state reached, entry/exit/effect traces in order, every guard branch), plus the events a state must ignore. For a `Deferred` row, assert that the event has no effect in the deferring state and takes effect after the next transition (see `DeferingMechanism` in `SimpleState`'s tests). For a history, assert that a transition into `H` restores the last active substate and that any other entry into the composite starts at its initial state (see the tests of `Urgent` and `Interrupt`). Update `main.cpp` for events that carry data and cannot be default-constructed.

---

### Step 5: Build, Test & Run

Run from the directory that contains `<StateMachineName>/`:

```bash
clang-format -i <StateMachineName>/src/*.?pp <StateMachineName>/tests/*.cpp
cmake -S <StateMachineName> -B <StateMachineName>/build
cmake --build <StateMachineName>/build -j8
ctest --test-dir <StateMachineName>/build --output-on-failure
./<StateMachineName>/build/<StateMachineName>
```

The build treats warnings as errors, so the code is not done until it compiles cleanly and every test passes. In the interactive application, press the keys listed at startup to stimulate the state machine, and check transitions and entry/exit/effect traces in real time.

#### Compilation database for clangd

The build exports `compile_commands.json` to `<StateMachineName>/build/`, where clangd finds it. For another build directory, link it from the project root: `ln -sf <dir>/compile_commands.json <StateMachineName>/`.

---

## Common Mistakes & Red Flags

| Mistake | Fix |
| :--- | :--- |
| Compile error deep inside `boost/mpl` with more than 20 rows or events | The scaffolded CMakeLists.txt raises the MPL limits to 50, the most MPL supports. Split a table with more than 50 rows into submachines. |
| Action or guard calls `isFlagActive()`/`waitForFlag()` of the active object | Deadlock: dispatch holds the state machine mutex. Read the state through the `fsm` argument instead. |
| Logger access crashes inside a composite state | `fsm` there is the composite itself. Give its front-end a `Log* m_logger` and set `m_logger = fsm.m_logger;` in its `on_entry`, which runs before its substates are entered. |
| Test expects `no_transition()` when a guard rejects | Boost.MSM ignores the event silently. Assert that the state and the trace are unchanged. |
| Two choice branches can be true at once | Make the guards mutually exclusive; MSM evaluates rows bottom-up. |
| A restored composite is expected to restore its nested composites too | Boost.MSM history is shallow: a nested composite without its own history policy restarts at its initial state. Give it its own policy only if the diagram has an `H` inside it. |
| Querying states by `current_state()` ids | Ids depend on table order. Use flags and `is_flag_active<Flag>()`. |
| Event posted from `main.cpp` has data members | Give it a default constructor or pass arguments: `post<Event::X>(args...)`. |

---

## Additional Resources

* Boost.MSM documentation and API lookup, and exploring the user's project
  with SocratiCode: [reference.md](reference.md)
* Reference examples and when to use each: [examples.md](examples.md)
* Project templates used by the script: `assets/templates/`

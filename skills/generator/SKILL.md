---
name: generator
description: This skill should be used when the user asks to "generate C++ code from a state diagram", "convert this statechart to Boost.MSM", "implement this UML state machine in C++", or provides a statechart or state diagram image and wants Boost.MSM code, an active object, unit tests or a CMake project for it.
---

# Generating Boost.MSM C++ Code from State Diagrams

## Overview

This guideline provides a step-by-step, fail-safe workflow to translate any state diagram into a warning-free, highly cohesive state machine based on the Boost.MSM library.

---

## The Workflow

### Step 1: Generate & Verify the UML Topology Table (Mandatory First Step)

Before writing any code, analyze the input statechart or state diagram image and translate its structure and behavior into a structured markdown table. The table must conform strictly to the following 9-column schema:

| States / Pseudo-state Names | Type | Ancestor | Entry Actions | Exit Actions | Transition Trigger | Guard | Action Effect | Next State |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |

> Note: Show the generated table to the user.

Then save the same table to `<StateMachineName>/<StateMachineName>.md`, where `<StateMachineName>` is the CamelCase name later passed to the scaffolding script and the directory is the one it will create (under `--output-dir`, if given; e.g., `SelectiveStart/SelectiveStart.md`). Use this layout:

```markdown
# <StateMachineName> Topology Table

Source diagram: `<diagram file name>`

<the table, exactly as shown to the user>
```

Writing it before Step 2 is safe: the scaffolding script never creates or overwrites this file, even with `--force`. If the user corrects the table, update the file so it always matches the table the implementation is built from.

#### Schema Formatting Rules

* **States / Pseudo-state Names**: Name of the state, substate, or pseudo-state (e.g., `[*]`, `S1`, `Idle`).
* **Type**: Type of state (e.g., `Initial Pseudo-state`, `Simple State`, `Composite State`, `Choice Pseudo-state`, `History Pseudo-state`, `Final State`).
* **Ancestor**: The parent state encapsulating this state. If the state is at the top-level (direct child of the main statechart), use a dash `—`.
* **Entry Actions**: Use `None` if empty.
* **Exit Actions**: Use `None` if empty.
* **Transition Trigger**: The event that triggers a transition. Use `None (Anonymous)` for event-less transitions, and a dash `—` if no outgoing transitions exist.
* **Guard**: Use `None` if empty. For the `[else]` branch of a choice, write `else`.
* **Action Effect**: Use `None` if empty, including on a `Deferred` row.
* **Next State**: The destination state of the transition. Use a dash `—` if no outgoing transitions are defined. Write the bare state name with no annotations, except for these two keywords:
  * `Internal`: an internal transition, which runs its effect without exiting or re-entering the state.
  * `Deferred`: the state defers the event in Transition Trigger.

  A transition from outside a composite state into one of its substates names the substate itself (e.g., `S41`, not `S4`); the substate's Ancestor column already shows that it is an entry into the composite.

#### Reading the Diagram

Identify each element's Type by its UML shape, not by its name or position:

| Shape | Type |
| :--- | :--- |
| Filled black circle | `Initial Pseudo-state` |
| Rounded rectangle | `Simple State`, or `Composite State` when it contains states |
| Diamond | `Choice Pseudo-state` |
| Circle with `H` / `H*` | `History Pseudo-state` (shallow / deep) |
| Circle with an `X` on a composite's border | `Exit Point` |
| Hollow circle on a composite's border | `Entry Point` |
| Filled circle inside a ring (bull's-eye) | `Final State` |

Elements drawn in red and struck through are the original UML intent that Boost.MSM cannot express directly; the black elements next to them are the intended implementation. Leave the red elements out of the table.

---

### Step 2: Scaffold the Project

Do not create the project files by hand. Run the bundled scaffolding script, located at `scripts/scaffold_statemachine.py` under this skill's base directory:

```bash
python3 <skill_base_dir>/scripts/scaffold_statemachine.py <StateMachineName> \
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

Before writing the implementation, consult the examples listed in [examples.md](examples.md) that match the features identified in the table from Step 1 (e.g., choice pseudo-states, hierarchy, deferred events, initial-transition actions).

---

### Step 4: Implement the State Machine (`src/<StateMachineName>.hpp`)

Replace every `TODO` of the skeleton with the content of the topology table:

1. Replace the placeholder `Idle` state with one `msm::front::state<>` per simple state. Implement each Entry/Exit Action in `on_entry`/`on_exit`. Trace through `fsm.m_logger`, so tests can assert on the trace order.
2. Add one functor per Action Effect and one `bool` functor per Guard.
   Write every `on_entry`/`on_exit` and every functor out in full, even when several look alike. Do not generate them with preprocessor macros (e.g. `TRACE_STATE(S1)`): macros hide the code from the reader, the debugger and clangd.
3. Add one `Row<Source, Event::Trigger, Target, Action, Guard>` per table row, using `none` for empty cells. The skeleton defines `Row` and `none` shorthands, visible in nested composites too.
   Start every transition table with the column header below, and align each row to its columns (keep the table between `// clang-format off` and `// clang-format on`):

   ```cpp
   struct transition_table : mpl::vector<
       //    Start     Event         Next      Action               Guard
       //  +---------+--------------+---------+---------------------+----------------------+
       Row < Idle    , Event::Start , Running , startProcess        , none                 >,
       Row < Running , Event::Stop  , Idle    , none                , none                 >
   > {};
   ```
4. Add a flag (`struct XFlag {}` plus `typedef mpl::vector1<XFlag> flag_list;`) to every state that the active object or the tests must query.
5. Add any event data that guards or actions read to `src/Event.hpp`.

Map each table Type as follows:

| Table Type | Boost.MSM construct |
| :--- | :--- |
| Simple State | `struct S : msm::front::state<>` |
| Composite State | Nested `struct S_ : msm::front::state_machine_def<S_>` plus `typedef msm::back::state_machine<S_> S;` |
| Initial Pseudo-state | `typedef S initial_state;` in the owning (sub)machine. An initial transition with an effect needs a transient state with an anonymous transition (see `Mgr` in examples.md). |
| Choice Pseudo-state | A plain state with anonymous rows (`Event` = `none`), one per branch, each with its guard. Guards must be mutually exclusive; implement an `else` guard as the negation of the other branches' guards. |
| History Pseudo-state | Back-end policy: `msm::back::state_machine<S_, msm::back::ShallowHistory<mpl::vector<Ev>>>` or `AlwaysHistory` |
| Final State | `msm::front::terminate_state<>` at the top level. Inside a composite, use an `exit_pseudo_state<Ev>` and `S::exit_pt<...>` row (see `SimpleState`). |
| Exit Point | `struct X : msm::front::exit_pseudo_state<Ev>` in the composite, plus a parent row from `S::exit_pt<S_::X>` (see `SimpleState`) |
| Entry Point / transition into a substate | Target substate also inherits `msm::front::explicit_entry<0>`; the row targets `S::direct<S_::Sub>` (see `SimpleState`) |
| Orthogonal regions | `typedef mpl::vector<R1Initial, R2Initial> initial_state;` |
| Internal transition | Row with `none` as Next State |
| Deferred event | `typedef mpl::vector<Ev> deferred_events;` in the state |

Then rewrite `tests/test_<StateMachineName>.cpp`. Write at least one test per table row (target state reached, entry/exit/effect traces in order, every guard branch), plus the events a state must ignore. Update `main.cpp` for events that carry data and cannot be default-constructed.

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

#### Generate compile DB for clangd

When editing from the host, run the following command on the host; otherwise run it within the builder container. Run it from the `<StateMachineName>/` directory:

```sh
cmake -H. -BDebug \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=YES && \
    ln -s Debug/compile_commands.json .
```

---

## Common Mistakes & Red Flags

| Mistake | Fix |
| :--- | :--- |
| Compile error deep inside `boost/mpl` with more than 20 rows or events | The scaffolded CMakeLists.txt raises the MPL limits to 50. Split larger tables into submachines. |
| Action or guard calls `isFlagActive()`/`waitForFlag()` of the active object | Deadlock: dispatch holds the state machine mutex. Read the state through the `fsm` argument instead. |
| Logger access crashes inside a composite state | `fsm` there is the composite itself. Give its front-end a `Log* m_logger` and set `m_logger = fsm.m_logger;` in its `on_entry`, which runs before its substates are entered. |
| Test expects `no_transition()` when a guard rejects | Boost.MSM ignores the event silently. Assert that the state and the trace are unchanged. |
| Two choice branches can be true at once | Make the guards mutually exclusive; MSM evaluates rows bottom-up. |
| Querying states by `current_state()` ids | Ids depend on table order. Use flags and `is_flag_active<Flag>()`. |
| Event posted from `main.cpp` has data members | Give it a default constructor or pass arguments: `post<Event::X>(args...)`. |

---

## Additional Resources

* Boost.MSM documentation and API lookup, and exploring the user's project
  with SocratiCode: [reference.md](reference.md)
* Reference examples and when to use each: [examples.md](examples.md)
* Project templates used by the script: `assets/templates/`

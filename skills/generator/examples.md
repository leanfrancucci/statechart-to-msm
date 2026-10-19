# 📚 Reference Examples

When implementing specific statechart features, consult the following state machine implementations within the `examples` directory as canonical reference examples. Each one has its state diagram next to it: `<Example>.png` for the examples at the top of `examples/`, and the file named below for the examples in their own folder. Elements drawn in red and struck through show the original UML intent that Boost.MSM cannot express directly; the black elements next to them are what the example implements instead.

* **`SimpleState`**: Consult this example when the state diagram has **multiple levels of hierarchy**, **exit pseudo-states** (a final state inside a composite), a **choice pseudostate**, a **deferred event** and **complex transitions**.
Source files: `SimpleState.h`, `SimpleState.cpp` and their dependencies (`Event.hpp`, `Event.cpp`). Tests: `tests/test_SimpleState.cpp`. Project file: `CMakeLists.txt`
* **`CompositeState`**: Consult this example when the state diagram has, at least, one **composite state**.
Source files: `CompositeState.h` and its dependencies. Tests: `tests/test_CompositeState.cpp`. Project file: `CMakeLists.txt`
* **`Mgr`**: Consult this example to generate an active object implementation.
Source files: `Mgr.hpp`, `Mgr.cpp` and their dependencies (`EventQueue.hpp`, `Log.hpp`). It also shows the workaround for an initial transition that carries an action (transient `Initial` state with an anonymous transition). Tests: `tests/test_Mgr.cpp` (logging sequence checked with a Google Mock `Log`) and `tests/test_EventQueue.cpp` (bounded queue: capacity, ownership, blocking pop, re-entrant push). Project file: `CMakeLists.txt`
* **`Urgent`** (folder `examples/Urgent/`): Consult this example when the state diagram has a **shallow history pseudo-state (`H`) that only some transitions target**. `Resume` targets the history inside the composite `Operation` and restores its last active substate, while `Reset` targets `Operation` itself and restarts it at `Idle`: `msm::back::ShallowHistory<mpl::vector<Event::Resume>>`.
Diagram: `urgent-state-machine.png`. Topology table, including the `H` row and a Next State `H`: `Urgent.md`. Source files: `src/Urgent.hpp`. Tests: `tests/test_Urgent.cpp` (history restored on `Resume`, initial state on `Reset`).
* **`Interrupt`** (folder `examples/Interrupt/`): Consult this example when the state diagram has a **history pseudo-state (`H`) as a composite's default entry**, so every entry into the composite restores its last active substate: `msm::back::AlwaysHistory`. It also shows that the history is shallow: the nested composite `Process` restarts at `Stage1` when `Operation` is restored.
Diagram: `interrupt-state-machine.png`. Topology table: `Interrupt.md`. Source files: `src/Interrupt.hpp`. Tests: `tests/test_Interrupt.cpp` (history restored on `Resume`, nested composite restarted).

Deep history (`H*`) has no example: Boost.MSM does not support it (see Reading the Diagram in SKILL.md).

> Note: The `CMakeLists.txt` and `tests/CMakeLists.txt` at the top of `examples/` are not standalone; the scaffolded project has its own. `Urgent` and `Interrupt` are complete scaffolded projects, so their project files match what the scaffolding script generates. `main.cpp` is a simple example to test the generated active object whose behaviour is represented by the inputted state machine.

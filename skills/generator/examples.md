# 📚 Reference Examples

When implementing specific statechart features, consult the following state machine implementations within the `examples` directory as canonical reference examples. Each one has its state diagram next to it (`<Example>.png`). Elements drawn in red and struck through show the original UML intent that Boost.MSM cannot express directly; the black elements next to them are what the example implements instead.

* **`SimpleState`**: Consult this example when the state diagram has **multiple levels of hierarchy**, **exit pseudo-states** (a final state inside a composite), a **choice pseudostate**, a **deferred event** and **complex transitions**.
Source files: `SimpleState.h`, `SimpleState.cpp` and their dependencies (`Event.hpp`, `Event.cpp`). Tests: `tests/test_SimpleState.cpp`. Project file: `CMakeLists.txt`
* **`CompositeState`**: Consult this example when the state diagram has, at least, one **composite state**.
Source files: `CompositeState.h` and its dependencies. Tests: `tests/test_CompositeState.cpp`. Project file: `CMakeLists.txt`
* **`Mgr`**: Consult this example to generate an active object implementation.
Source files: `Mgr.hpp`, `Mgr.cpp` and their dependencies (`EventQueue.hpp`, `Log.hpp`). It also shows the workaround for an initial transition that carries an action (transient `Initial` state with an anonymous transition). Tests: `tests/test_Mgr.cpp` (logging sequence checked with a Google Mock `Log`) and `tests/test_EventQueue.cpp` (bounded queue: capacity, ownership, blocking pop, re-entrant push). Project file: `CMakeLists.txt`

> Note: `CMakeLists.txt` and `tests/CMakeLists.txt` here are not standalone; the scaffolded project has its own. `main.cpp` is a simple example to test the generated active object whose behaviour is represented by the inputted state machine.

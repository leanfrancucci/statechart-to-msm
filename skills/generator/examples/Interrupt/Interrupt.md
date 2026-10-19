# Interrupt Topology Table

Source diagram: `interrupt-state-machine.png`

| States / Pseudo-state Names | Type | Ancestor | Entry Actions | Exit Actions | Transition Trigger | Guard | Action Effect | Next State |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `[*]` | Initial Pseudo-state | — | None | None | None (Anonymous) | None | None | Operation |
| Operation | Composite State | — | None | None | Signal | None | None | Paused |
| `[*]` | Initial Pseudo-state | Operation | None | None | None (Anonymous) | None | None | H |
| H | History Pseudo-state | Operation | None | None | None (Anonymous) | None | None | Idle |
| Idle | Simple State | Operation | None | None | Start | None | None | Process |
| Process | Composite State | Operation | None | None | — | None | None | — |
| `[*]` | Initial Pseudo-state | Process | None | None | None (Anonymous) | None | None | Stage1 |
| Stage1 | Simple State | Process | None | None | Input | None | None | Stage2 |
| Stage2 | Simple State | Process | None | None | Input | None | None | Stage1 |
| Paused | Simple State | — | suspend() | None | Resume | None | None | Operation |

# Urgent Topology Table

Source diagram: `urgent-state-machine.png`

| States / Pseudo-state Names | Type | Ancestor | Entry Actions | Exit Actions | Transition Trigger | Guard | Action Effect | Next State |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `[*]` | Initial Pseudo-state | — | None | None | None (Anonymous) | None | None | Operation |
| Operation | Composite State | — | None | None | Signal | None | None | Paused |
| `[*]` | Initial Pseudo-state | Operation | None | None | None (Anonymous) | None | None | Idle |
| Idle | Simple State | Operation | None | None | Start | None | None | Processing |
| Processing | Simple State | Operation | None | None | Stop | None | None | Idle |
| H | History Pseudo-state | Operation | None | None | — | None | None | — |
| Paused | Simple State | — | suspend() | None | Resume | None | None | H |
| Paused | Simple State | — | suspend() | None | Reset | None | None | Operation |

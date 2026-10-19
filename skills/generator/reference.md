# API References & Documentation

If unsure about a Boost.MSM concept or API, **DO NOT GUESS**:

1. Read the headers of the installed Boost. Their location depends on the system: `/usr/include/boost/` on Debian/Ubuntu, otherwise the `Boost_DIR` that CMake resolved (see `build/CMakeCache.txt`). For example, `/usr/include/boost/msm/front/` (front-ends, states, functor rows) and `/usr/include/boost/msm/back/state_machine.hpp` (back-end, history policies, flags).
2. Fetch the official documentation that matches the installed version (check `BOOST_LIB_VERSION` in `/usr/include/boost/version.hpp`), e.g. `https://www.boost.org/doc/libs/1_83_0/libs/msm/doc/HTML/ch03.html` for 1.83.

## Exploring the user's project with SocratiCode

SocratiCode is for exploring **the user's project**: the code the generated
state machine must fit into, such as existing events, active objects, logger
and queue classes, and how they are used. It is not a source of truth for
Boost.MSM itself; Boost headers under `/usr/include` are not indexed, so use
the steps above for the Boost.MSM API.

If the SocratiCode MCP server is available, **you MUST prioritize it** over
text search (Grep, `rg`) when exploring the user's project. It provides a
semantic understanding of the codebase that plain text search cannot.

Tool names carry an install-dependent prefix (e.g.
`mcp__plugin_socraticode_socraticode__codebase_symbol` for the plugin), so
match them by suffix:

* `codebase_status` — check first. Rely on semantic search only once the index
  is complete; otherwise run `codebase_index` or fall back to text search.
* `codebase_symbol` — 360-degree view of a specific function, type, or class
  in the project (definition, callers, callees).
* `codebase_search` — semantic queries over the project, e.g. "Where are
  events posted to the active object?" or "How does this project defer an
  event?".

## Exploring the user's project with CodeGraph

CodeGraph is, like SocratiCode, for exploring **the user's project**, not
Boost.MSM. It serves a pre-built graph of the project's symbols and their
relationships (calls, includes, inheritance), which answers structural
questions in one call instead of a chain of file reads.

If the CodeGraph MCP server is available, **prefer it** over text search
(Grep, `rg`) for structural questions about the user's project. Tool names
carry an install-dependent prefix (e.g. `mcp__codegraph__codegraph_explore`),
so match them by suffix:

* `codegraph_explore` — the default tool. Ask it how a part of the project
  works, e.g. "How are events posted to and dispatched by the active
  object?" or "Which classes derive from Log?".
* `codegraph_status`, `codegraph_search`, `codegraph_node`,
  `codegraph_callers`, `codegraph_callees`, `codegraph_impact` — use them if
  they are listed; the user enables them with `CODEGRAPH_MCP_TOOLS`.

CodeGraph only knows a project that has been indexed: the project root has a
`.codegraph/` directory. If it has none, or the tools report no index, ask
the user to run `codegraph init` in the project root, or fall back to the
other tools. Do not create the index yourself: it writes into the user's
project.

## Choosing between SocratiCode and CodeGraph

When both are available, use each for what it does best:

* **SocratiCode** for conceptual questions, where the right symbol name is
  not known yet: "How does this project defer an event?".
* **CodeGraph** for structural questions about known symbols: callers,
  callees, call flow, and the impact of changing a shared class such as the
  event queue or the logger.

When only one is available, use it for both kinds of question. Fall back to
text search only when neither is available or indexed.

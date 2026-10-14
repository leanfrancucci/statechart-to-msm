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

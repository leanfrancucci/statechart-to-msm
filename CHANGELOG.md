# Changelog

All notable changes to this plugin are documented here. This project uses
[Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/), [Semantic Versioning](https://semver.org/) and [Common Changelog](https://common-changelog.org/).

## [1.2.0] - 2026-10-14

### Changed

- Stop after the topology table and wait for the user to confirm or correct it before scaffolding ([`17306d2`](https://github.com/leanfrancucci/statechart-to-msm/commit/17306d2))
- Defer events with `Defer` rows only, marked in the table with the Action Effect `Defer` and the Next State `Deferred` ([`17306d2`](https://github.com/leanfrancucci/statechart-to-msm/commit/17306d2))
- Accept the state diagram as an image only, and trigger on more phrasings but not for other state machine libraries or non-C++ targets ([`17306d2`](https://github.com/leanfrancucci/statechart-to-msm/commit/17306d2))

### Added

- Add rules for which table rows become transitions, which transition table each goes into, and how transitions enter or leave a composite state ([`17306d2`](https://github.com/leanfrancucci/statechart-to-msm/commit/17306d2))
- Add table rules for repeated rows, self-transitions and event names, with a worked example table ([`17306d2`](https://github.com/leanfrancucci/statechart-to-msm/commit/17306d2))
- Add a toolchain check before scaffolding ([`17306d2`](https://github.com/leanfrancucci/statechart-to-msm/commit/17306d2))

### Removed

- Remove the host-specific clangd compile-database instructions ([`17306d2`](https://github.com/leanfrancucci/statechart-to-msm/commit/17306d2))

## [1.1.0] - 2026-10-14

### Changed

- Define a fixed topology-table notation for internal transitions, deferred events, direct entry into a substate and a choice's else branch ([`3423355`](https://github.com/leanfrancucci/statechart-to-msm/commit/3423355))

### Added

- Document installing from the full repository URL ([`b0240d1`](https://github.com/leanfrancucci/statechart-to-msm/commit/b0240d1))

## [1.0.0] - 2026-10-14

🌱 Initial release.

[1.2.0]: https://github.com/leanfrancucci/statechart-to-msm/releases/tag/v1.2.0
[1.1.0]: https://github.com/leanfrancucci/statechart-to-msm/releases/tag/v1.1.0
[1.0.0]: https://github.com/leanfrancucci/statechart-to-msm/releases/tag/v1.0.0

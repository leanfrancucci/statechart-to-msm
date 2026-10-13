#!/usr/bin/env python3
"""Scaffold a CMake project for a Boost.MSM state machine.

Copies the templates under ../assets/templates into <output-dir>/<Name>/,
renaming files that contain __NAME__ and expanding the {{...}} placeholders.
The result builds and passes its tests as is: a placeholder state machine
(one state, empty transition table), an active object around it, an
interactive main.cpp with one key per event, and GoogleTest unit tests.

Usage:
    scaffold_statemachine.py <Name> [--events E1,E2,...] [--output-dir DIR]
                             [--force]

Exit status: 0 on success, 1 on invalid arguments, 2 if the destination
already contains files that would be overwritten (use --force).
"""

import argparse
import re
import sys
from pathlib import Path

TEMPLATES_DIR = Path(__file__).resolve().parent.parent / "assets" / "templates"

CAMEL_CASE = re.compile(r"^[A-Z][A-Za-z0-9]*$")
PLACEHOLDER = re.compile(r"\{\{[A-Z_]+\}\}")

# Names already taken by the scaffolded support code
RESERVED_NAMES = {"Event", "EventQueue", "Log", "ConsoleLogger", "DispatchVisitor"}

# Keys bound to events in main.cpp, in assignment order; 'h' and 'q' are
# taken by help and quit
EVENT_KEYS = "123456789" + "abcdefgijklmnoprstuvwxyz"

# BOOST_MPL_LIMIT_LIST_SIZE set in the generated CMakeLists.txt; the events
# variant cannot hold more alternatives
MAX_EVENTS = 50


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description="Scaffold a CMake project for a Boost.MSM state machine."
    )
    parser.add_argument(
        "name", help="CamelCase name of the state machine, e.g. SelectiveStart"
    )
    parser.add_argument(
        "--events",
        default="",
        help="comma-separated CamelCase event (trigger) names from the topology "
        "table, e.g. Start,Stop,Timeout (default: none)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path.cwd(),
        help="directory in which the <Name>/ project directory is created "
        "(default: current directory)",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="overwrite files that already exist in the project directory",
    )
    return parser.parse_args(argv)


def validate(name, events):
    """Returns a list of error messages; empty when the inputs are valid."""
    errors = []
    if not CAMEL_CASE.match(name):
        errors.append(f"state machine name '{name}' is not CamelCase (e.g. SelectiveStart)")
    elif name in RESERVED_NAMES:
        errors.append(f"state machine name '{name}' clashes with the generated support code")

    if len(events) > MAX_EVENTS:
        errors.append(f"{len(events)} events exceed the Boost.MPL limit of {MAX_EVENTS}")
    seen = set()
    for event in events:
        if not CAMEL_CASE.match(event):
            errors.append(f"event name '{event}' is not CamelCase (e.g. Timeout)")
        elif event in seen:
            errors.append(f"event '{event}' is listed more than once")
        elif event == name or event in RESERVED_NAMES:
            errors.append(f"event name '{event}' clashes with a generated type")
        seen.add(event)
    return errors


def event_structs(events):
    return "\n\n".join(f"struct {event}\n{{\n}};" for event in events)


def key_bindings(events):
    """Returns (bindings, unbound) where bindings pairs each key with an event."""
    bindings = list(zip(EVENT_KEYS, events))
    return bindings, events[len(bindings):]


def key_help(bindings):
    return "\n".join(
        f"              << \"  {key} - send '{event}'\\n\"" for key, event in bindings
    )


def key_cases(bindings):
    return "\n".join(
        f"            case '{key}':\n"
        f"                send<Event::{event}>(actObj, \"{event}\");\n"
        f"                break;"
        for key, event in bindings
    )


def render(text, values):
    for key, value in values.items():
        text = text.replace("{{" + key + "}}", value)
    leftover = PLACEHOLDER.search(text)
    if leftover:
        raise ValueError(f"unexpanded placeholder {leftover.group(0)}")
    return text


def plan_files(name, project_dir):
    """Maps every template to the path it is rendered to."""
    plan = []
    for template in sorted(TEMPLATES_DIR.rglob("*")):
        if template.is_file():
            relative = template.relative_to(TEMPLATES_DIR).as_posix()
            plan.append((template, project_dir / relative.replace("__NAME__", name)))
    return plan


def main(argv=None):
    args = parse_args(sys.argv[1:] if argv is None else argv)
    name = args.name
    events = [e.strip() for e in args.events.split(",") if e.strip()]

    errors = validate(name, events)
    if errors:
        for error in errors:
            print(f"error: {error}", file=sys.stderr)
        return 1

    if not TEMPLATES_DIR.is_dir():
        print(f"error: templates directory not found: {TEMPLATES_DIR}", file=sys.stderr)
        return 1

    project_dir = (args.output_dir / name).resolve()
    plan = plan_files(name, project_dir)

    existing = [dest for _, dest in plan if dest.exists()]
    if existing and not args.force:
        print(f"error: {project_dir} already contains generated files:", file=sys.stderr)
        for dest in existing:
            print(f"  {dest.relative_to(project_dir)}", file=sys.stderr)
        print("re-run with --force to overwrite them", file=sys.stderr)
        return 2

    bindings, unbound = key_bindings(events)
    values = {
        "NAME": name,
        "NAME_UPPER": re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", name).upper(),
        "EVENT_STRUCTS": event_structs(events),
        # boost::variant needs at least one alternative
        "EVENT_LIST": ", ".join(f"Event::{event}" for event in events) or "boost::blank",
        "FIRST_EVENT_TYPE": f"Event::{events[0]}" if events else "boost::blank",
        "KEY_HELP": key_help(bindings),
        "KEY_CASES": key_cases(bindings),
    }

    for template, dest in plan:
        try:
            content = render(template.read_text(encoding="utf-8"), values)
        except ValueError as exc:
            print(f"error: {template.relative_to(TEMPLATES_DIR)}: {exc}", file=sys.stderr)
            return 1
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_text(content, encoding="utf-8")

    print(f"Scaffolded {name} in {project_dir}")
    for _, dest in plan:
        print(f"  {dest.relative_to(project_dir)}")
    if unbound:
        print(
            "warning: no key left in main.cpp for: " + ", ".join(unbound),
            file=sys.stderr,
        )
    print(
        "\nNext: implement src/{0}.hpp and rewrite tests/test_{0}.cpp from the "
        "topology table, then build:\n"
        "  cmake -S {1} -B {1}/build && cmake --build {1}/build -j\n"
        "  ctest --test-dir {1}/build --output-on-failure".format(name, project_dir)
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())

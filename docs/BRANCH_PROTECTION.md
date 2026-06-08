# Branch protection (GitHub settings)

CI alone does **not** block direct pushes to `main`. Configure branch protection in the repo on GitHub:

1. **Settings** → **Branches** → **Add branch ruleset** (or **Add rule** for `main`)
2. Enable:
   - **Require a pull request before merging**
   - **Require approvals** (at least 1)
   - **Require status checks to pass before merging**
   - Select these checks after the workflow has run once:
     - `Build & test (Make)`
     - `Build & test (CMake)`
   - **Do not allow bypassing the above settings** (recommended)
3. Optionally: **Restrict who can push to matching branches** so only admins or nobody can push directly.

## Workflow

```text
feature/my-change  →  open PR to main  →  CI runs  →  review + approve  →  merge
```

Direct push to `main` should be rejected once the ruleset is active.

## Local commands (same as CI)

```bash
make
make test
```

Or with CMake:

```bash
cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure
```

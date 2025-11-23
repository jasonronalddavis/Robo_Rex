# Resolving "Need to specify how to reconcile divergent branches"

Git emits this message when `git pull` needs to know whether to merge or rebase.

## Quick fix (merge strategy)
Run once in the repo:

```bash
git config pull.rebase false
```

## Alternative (rebase on pull)
If you prefer rebasing instead of merge commits:

```bash
git config pull.rebase true
```

Either command sets the default so future `git pull` invocations proceed without the prompt.

## Force update to the latest remote state
If you want your local branch to exactly match the latest code on `origin/main` (discarding local changes), run:

```bash
git fetch origin
git reset --hard origin/main
# Optional: remove untracked files created locally
git clean -fd
```

## Bring your work branch up to date while keeping local changes
To update your feature branch to the latest `origin/main` while preserving edits:

```bash
git fetch origin
# Option 1: rebase to replay your changes on top of main
git rebase origin/main
# Option 2: merge main into your branch instead of rebasing
git merge origin/main
```

If conflicts appear, resolve them in each file, run `git add <file>`, then continue with `git rebase --continue` or complete the merge with `git commit`.

# git hooks

## Implemented Hooks

- [pre-commit](./hooks/pre-commit): runs linting and formatting on staged JavaScript and TypeScript files
- [pre-push](./hooks/pre-push): prevents pushes to the main branch

## Install Hooks
The script will copy the hooks to [.git/hooks](/.git/hooks/) and make the hooks executable.

⚠️ Note that this will replace any existing hooks with the same name in your local project! ⚠️

💡 Windows: run the commands from a `Git Bash` terminal.

From this directory (`git_hooks`), use the commands below to make the script executable and then run [set-up-git-hooks](./set-up-git-hooks).

```sh
chmod +x ./set-up-git-hooks
./set-up-git-hooks
```

## Uninstall Hooks

Go to [.git/hooks](/.git/hooks/) and delete the file for the hook you want to uninstall.
- eg. If you want to uninstall the `pre-commit` hook, delete the `.git/hooks/pre-commit` file.

> If you are using VSCode and don't see the `.git` folder in your file explorer, go to VSCode settings and search for "exclude". You may need to remove `.git` from the exclude list for the file explorer.

## Skip Hooks
Hooks can be skipped by appending `--no-verify` to the git command you want to run without its corresponding hook.
- eg. To skip the pre-commit hook, you'd use `git commit --no-verify`.

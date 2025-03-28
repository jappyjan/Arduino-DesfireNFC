#!/bin/bash

# Print environment information
echo "=== Environment Information ==="
echo "Shell: $SHELL"
echo "PATH: $PATH"
echo "Git version: $(git --version)"
echo "Current directory: $(pwd)"
echo ""

# Check if Git hooks directory is properly configured
echo "=== Git Hooks Configuration ==="
HOOKS_PATH=$(git config core.hooksPath)
echo "Configured hooks path: $HOOKS_PATH"
echo ""

# Check pre-commit hook permissions and content
echo "=== Pre-commit Hook Check ==="
if [ -n "$HOOKS_PATH" ] && [ -f "$HOOKS_PATH/pre-commit" ]; then
  echo "pre-commit hook exists"
  ls -la "$HOOKS_PATH/pre-commit"
  echo "First 5 lines of pre-commit hook:"
  head -n 5 "$HOOKS_PATH/pre-commit"
  
  # Verify it's executable
  if [ -x "$HOOKS_PATH/pre-commit" ]; then
    echo "pre-commit hook is executable"
  else
    echo "pre-commit hook is NOT executable"
  fi
else
  echo "pre-commit hook doesn't exist or hooks path is not configured"
fi
echo ""

# Try running the hook directly
echo "=== Running Pre-commit Hook Directly ==="
if [ -n "$HOOKS_PATH" ] && [ -f "$HOOKS_PATH/pre-commit" ]; then
  echo "Attempting to run the pre-commit hook directly..."
  "$HOOKS_PATH/pre-commit"
  EXIT_CODE=$?
  echo "Exit code: $EXIT_CODE"
else
  echo "Cannot run pre-commit hook: file doesn't exist"
fi
echo ""

# Check if our debug hook works
echo "=== Testing Debug Hook ==="
if [ -f ".githooks/pre-commit-debug" ]; then
  echo "Debug hook exists"
  chmod +x .githooks/pre-commit-debug
  echo "Running debug hook directly..."
  .githooks/pre-commit-debug
  EXIT_CODE=$?
  echo "Debug hook exit code: $EXIT_CODE"
  echo "Debug log contents:"
  cat /tmp/pre-commit-debug.log
else
  echo "Debug hook doesn't exist"
fi

# Suggest next steps
echo ""
echo "=== Next Steps ==="
echo "1. If the hooks path is not configured, run: ./setup-git-hooks.sh"
echo "2. If the pre-commit hook is not executable, run: chmod +x $HOOKS_PATH/pre-commit"
echo "3. Try using the debug hook instead: cp .githooks/pre-commit-debug $HOOKS_PATH/pre-commit"
echo "4. Check for errors in the debug hook log: /tmp/pre-commit-debug.log" 
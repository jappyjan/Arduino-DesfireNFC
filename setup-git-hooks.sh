#!/bin/bash
set -e

# Configure Git to use our custom hooks directory
git config core.hooksPath .githooks

echo "Git hooks setup complete. Pre-commit linting is now active." 
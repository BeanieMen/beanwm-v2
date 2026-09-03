#!/usr/bin/env bash
set -euo pipefail

REMOTE="origin"
BRANCH="master"

# Make sure we're on the latest code
git fetch "$REMOTE"
git checkout "$BRANCH"
git pull --ff-only "$REMOTE" "$BRANCH"

# Get the latest GitHub release/tag
LATEST_TAG=$(gh release view --json tagName -q '.tagName')

if [[ -z "$LATEST_TAG" ]]; then
    echo "ERROR: Could not determine latest GitHub release."
    exit 1
fi

# Get current commit
LATEST_COMMIT=$(git rev-parse "$BRANCH")

echo "Latest release: $LATEST_TAG"
echo "Updating it to commit: $LATEST_COMMIT"

# Delete the existing remote tag
git push "$REMOTE" --delete "$LATEST_TAG" || true

# Delete local tag if it exists
git tag -d "$LATEST_TAG" 2>/dev/null || true

# Recreate tag on latest commit
git tag -a "$LATEST_TAG" "$LATEST_COMMIT" -m "Release $LATEST_TAG"

# Push updated tag
git push "$REMOTE" "$LATEST_TAG"

# Delete/recreate GitHub release so it points to the new tag
gh release delete "$LATEST_TAG" --yes

gh release create "$LATEST_TAG" \
    --title "beanwm $LATEST_TAG" \
    --generate-notes

echo
echo "======================================"
echo " Updated release: $LATEST_TAG"
echo " Commit: $LATEST_COMMIT"
echo "======================================"
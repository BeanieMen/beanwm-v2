#!/usr/bin/env bash
set -euo pipefail

PKGBUILD="PKGBUILD"
REMOTE="origin"
BRANCH="master"

# Make sure we're on the latest remote code
git fetch "$REMOTE"
git checkout "$BRANCH"
git pull --ff-only "$REMOTE" "$BRANCH"

# Bump patch version
pkgver=$(grep '^pkgver=' "$PKGBUILD" | cut -d= -f2)

IFS='.' read -r major minor patch <<< "$pkgver"
newver="$major.$minor.$((patch + 1))"

echo "Bumping $pkgver -> $newver"

# Update version
sed -i "s/^pkgver=.*/pkgver=$newver/" "$PKGBUILD"
sed -i "s/^pkgrel=.*/pkgrel=1/" "$PKGBUILD"

# Commit version bump
git add "$PKGBUILD"
git commit -m "release: v$newver"

# Push latest code
git push "$REMOTE" "$BRANCH"

# Create tag on the exact commit we just pushed
git tag -a "v$newver" -m "Release v$newver"

# Push tag
git push "$REMOTE" "v$newver"

# Create GitHub Release
gh release create "v$newver" \
    --title "beanwm v$newver" \
    --generate-notes

# Clean previous build/source/package files in isolated build dir
BUILD_TMP="/tmp/makepkg-beanwm"
rm -rf "$BUILD_TMP"
mkdir -p "$BUILD_TMP"

# Now the GitHub archive actually exists, so checksums work
updpkgsums

# Build and install using isolated BUILDDIR so repo directory and src/ are never touched
BUILDDIR="$BUILD_TMP" PKGDEST="$BUILD_TMP" SRCDEST="$BUILD_TMP" makepkg -si -c

echo
echo "======================================"
echo " Released beanwm v$newver"
echo " Tag: v$newver"
echo "======================================"
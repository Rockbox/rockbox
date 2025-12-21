#!/bin/bash
set -eu

UPSTREAM="https://github.com/amachronic/reggen"
temp_dir="$(mktemp -d)"
repo_dir="${temp_dir}/reggen"
dest_dir=$(dirname "$0")

trap 'echo rm -rf "$temp_dir"' EXIT

git clone "$UPSTREAM" "$repo_dir"

rm -f "$dest_dir"/*.[ch] "$dest_dir"/*.md "$dest_dir"/*.txt
cp "$repo_dir"/src/*.[ch] -t "$dest_dir"
cp "$repo_dir"/README.md -t "$dest_dir"
cp "$repo_dir"/LICENSE-GPLv3.txt -t "$dest_dir"

git -C "$repo_dir" rev-parse HEAD > "$dest_dir"/upstream-commit

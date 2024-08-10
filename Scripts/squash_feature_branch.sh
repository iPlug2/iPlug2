#!/bin/bash

# Function to display recent branches
display_recent_branches() {
  echo "Here are the 10 most recently updated branches:"
  git for-each-ref --sort=-committerdate refs/heads/ --format='%(refname:short)' | head -n 10
  echo ""
  echo "Please run the script again with a branch name as an argument."
  exit 0
}

# Check if a branch name was provided
if [ $# -eq 0 ]; then
  echo "Error: Please provide a branch name to squash merge."
  echo ""
  display_recent_branches
fi

feature_branch="$1"

# Check if the feature branch exists
if ! git rev-parse --verify "$feature_branch" >/dev/null 2>&1; then
  echo "Error: Branch '$feature_branch' does not exist."
  display_recent_branches
fi

# Check for unmerged changes
if ! git diff-index --quiet HEAD; then
  echo "Error: You have uncommitted changes in tracked files."
  echo "Please commit or stash these changes before proceeding."
  exit 1
fi

# Get the current branch name
current_branch=$(git rev-parse --abbrev-ref HEAD)

# Perform the squash merge
git merge --squash "$feature_branch"

# Check if the merge was successful
if [ $? -ne 0 ]; then
  echo "Error: Merge failed. Please resolve conflicts and try again."
  exit 1
fi

# Create the commit with the specified message format
git commit -m "[DROP] Squash of $feature_branch"

echo "Squash merge of '$feature_branch' into '$current_branch' completed successfully."
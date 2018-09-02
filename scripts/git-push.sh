#!/usr/bin/env bash

# This script assumes "bitbucket" and "gitlab" remotes have already been added.

git push -u bitbucket master
git push -u gitlab master


#!/bin/bash

git submodule update --init --recursive

# Checkout imgui docking branch
cd vendor/imgui
git checkout docking
git pull origin docking
cd ../..
#!/bin/bash
set -e

# Install dependencies
apt-get update
apt-get install -y tree-sitter-cli

# Install CodeT5
pip install gdown
mkdir bugfix_pretrain_with_ast
cd bugfix_pretrain_with_ast
gdown 1057u16sqSf14w51CA0fZt-WJ6FjS2X6I  # CodeT5 model
cd ..

# Unzip compressed files
cd vulfix_data
gunzip -k train.json.gz
cd ..
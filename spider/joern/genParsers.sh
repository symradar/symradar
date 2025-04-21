#!/bin/bash
#antlr4='java -jar ./antlr4-4.2.1-SNAPSHOT-complete.jar'
antlr4='java -jar ./antlr-4.2.1-complete.jar'

$antlr4 src/antlr/C/Module.g4
$antlr4 src/antlr/C/Function.g4


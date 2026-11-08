#!/usr/bin/env bash
set -euo pipefail

NAME=BlagoySimandov_123776895
DIST=deliverables/$NAME

(cd report && mkdir -p build && tectonic -X compile main.tex --outdir build)
rm -rf "$DIST" "deliverables/$NAME.zip"
mkdir -p "$DIST/doc"
cp sorting.c Makefile "$DIST/"
make -C "$DIST" sorting.o
cp report/build/main.pdf "$DIST/doc/Readme.pdf"
(cd deliverables && zip -rq "$NAME.zip" "$NAME")

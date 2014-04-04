#!/usr/bin/env bash

dia -e TMPFILE -t eps $1.dia
ps2pdf -dEPSCrop TMPFILE $1.pdf
rm TMPFILE

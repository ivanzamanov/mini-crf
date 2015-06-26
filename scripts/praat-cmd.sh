#!/bin/bash

praat $(readlink -f $1) $(readlink -f $2) $(readlink -f $3)

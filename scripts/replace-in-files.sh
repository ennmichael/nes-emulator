#!/usr/bin/env bash

function replace
{
        find $1 -name "*.cpp" -type f -exec sed -i s/$2/$3/g {} +
        find $1 -name "*.h" -type f -exec sed -i s/$2/$3/g {} +
}

replace src $1 $2
replace tests $1 $2


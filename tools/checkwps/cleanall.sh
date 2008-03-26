#!/bin/sh
cat targets.txt | (
    while read target model
    do
        rm -f checkwps.$model
    done
)

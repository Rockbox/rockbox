#!/bin/sh
cat targets.txt | (
    while read target model
    do
        rm -f checkwps.$model
        make MODEL=$model TARGET=$target
    done
)

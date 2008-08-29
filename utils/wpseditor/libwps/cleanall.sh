#!/bin/sh
cat targets.txt | (
    while read target model
    do
        make MODEL=$model TARGET=$target clean
    done
)

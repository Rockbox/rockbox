BEGIN { FS="[|)]" }

/^[ \t]*([0-9]+)\|([^)]+)\)$/ {
    model=$2
}

/^[ \t]*target="[^"]+"$/ {
    match($0, "=\".+\"")
    target=substr($0, RSTART+2, RLENGTH-3)
    print target, model
}

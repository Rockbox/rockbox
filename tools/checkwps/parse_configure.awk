BEGIN { FS="[|)]" }

/^[ \t]*([0-9]+)\|([^)]+)\)$/ {
    model=$2
}

/^[ \t]*target="[^"]+"$/ {
    match($0, "-D[^\"]+")
    target=substr($0, RSTART+2, RLENGTH-2)
    print target, model
}

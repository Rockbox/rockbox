echo "duplicates in models.txt:"
MODELS=$(cat models.txt | awk -F ',' '{ print($1) }' | sort)
echo "$MODELS" | uniq -d
echo "duplicates in series.txt:"
SERIES_MODELS=$(cat series.txt | awk '{ n=split($0,a,","); for(i=3;i<=n;i++) if(length(a[i]) != 0) print(a[i]); }' | sort)
echo "$SERIES_MODELS" | uniq -d
echo "diff between models.txt and series.txt:"
diff <(echo "$MODELS") <(echo "$SERIES_MODELS")

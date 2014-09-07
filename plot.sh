for a in a b c; do
    export VAR_DESC=$a-sz-$1
    gnuplot var.gnu &>/dev/null
    mv out.png $a.png
done
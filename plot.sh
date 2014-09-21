for a in "$@"; do
    export VAR_DESC=$a
    echo $a
    gnuplot var.gnu &>/dev/null
    mv out.png $a.png
done
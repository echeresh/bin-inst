for a in "$@"; do
    export VAR_DESC=$a
    echo $a
    gnuplot var.gnu
    mv out.png $a.png
done
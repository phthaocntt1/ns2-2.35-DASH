BEGIN {
FS=":"
}
(NR<10)||(NR>100) {print $1}


BEGIN{
FS=":"
}
$5 ~ /root/ {print $3}
# !

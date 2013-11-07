BEGIN{x=0}
/^$/ {x=x+1}
END {print "I found "x" blank lines."}

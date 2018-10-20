#!/bin/bash

FILE=${1:-nofile}
BAR=${FILE}-temp

translate-to-capo $FILE > /dev/null	
echo HGraphWPins : $FILE.nets $FILE.nodes > $FILE.aux
LayoutGen-Linux.exe -f $FILE.aux -saveAs $BAR > layoutgen.log 2>&1
echo "Capo: Begin"
MetaPl-Capo8.7_linux.exe -save -f $BAR.aux > capo.log 2>&1
echo "Capo: End"

mv out.pl $FILE.pl

rm -f $FILE.nets $FILE.nodes $FILE.aux
rm -f $BAR.nets $BAR.nodes $BAR.aux $BAR.scl $BAR.wts $BAR.pl
rm -f out-1.pl
rm -f seeds.out


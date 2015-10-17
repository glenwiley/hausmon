#!/bin/bash

fndata=/home/gwiley/owbmon/furnace.data
fnhtml=/var/www/html/furnace.html
# this is an adjustment for temps that get measured through
# a couling rather than directly.  We have to account for
# the insulating property of the coupler.
tempadjust="1.05"

set -- $(tail -1 $fndata)
dt="$1"
node="$2"
tempambC="$3"
tempcoldC="$4"
temphotC="$5"
power="$6"

temphotC=$(echo "$temphotC * $tempadjust" | bc) 
tempcoldC=$(echo "$tempcoldC * $tempadjust" | bc) 

tempambF=$(echo "$tempambC * 1.8 + 32" | bc)
temphotF=$(echo "$temphotC * 1.8 + 32" | bc)
tempcoldF=$(echo "$tempcoldC * 1.8 + 32" | bc)

echo "<html>" > $fnhtml
echo "<head>" >> $fnhtml
echo "<title>Furnance Monitor</title>" >> $fnhtml
echo "</head>" >> $fnhtml
echo "<body>" >> $fnhtml
echo "Time: $dt" >> $fnhtml
echo "<p>" >> $fnhtml
echo "Current Ambient Temperature: ${tempambF}F / ${tempambC}C" >> $fnhtml
echo "<p>" >> $fnhtml
echo "Current Furnace Water Temperature: ${temphotF}F / ${temphotC}C" >> $fnhtml
echo "<p>" >> $fnhtml
echo "Current Return Water Temperature: ${tempcoldF}F / ${tempcoldC}C" >> $fnhtml
echo "<p>" >> $fnhtml

if (( power > 30 || ( power < 20 && power > 10 ) ))
then
	dstate="on"
else
	dstate="off"
fi
echo "Damper State: $dstate" >> $fnhtml
echo "<p>" >> $fnhtml

if (( power > 20 ))
then
	pstate="on"
else
	pstate="off"
fi
echo "Pump State: $pstate" >> $fnhtml
echo "<p>" >> $fnhtml

echo "</body>" >> $fnhtml
echo "</html>" >> $fnhtml

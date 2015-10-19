#!/bin/bash

webdir=/var/www/html

fndata=/home/gwiley/owbmon/furnace.data
fnhtml=$webdir/furnace.html
fnsyshtml=/home/gwiley/owbmon/owbsystem.html
# this is an adjustment for temps that get measured through
# a couling rather than directly.  We have to account for
# the insulating property of the coupler.
tempcpladj="1.05"

set -- $(tail $fndata | grep furnace | tail -1)
dtfurn="$1"
node="$2"
tempambC="$3"
tempfurncoldC="$4"
tempfurnhotC="$5"
power="$6"

dtstr="${dtfurn:0:4}/${dtfurn:4:2}/${dtfurn:6:2} ${dtfurn:8:2}:${dtfurn:10:2}:${dtfurn:12:2}"
tempfurnhotC=$(echo "$tempfurnhotC * $tempcpladj" | bc) 
tempfurncoldC=$(echo "$tempfurncoldC * $tempcpladj" | bc) 

tempambF=$(echo "$tempambC * 1.8 + 32" | bc)
tempfurnhotF=$(echo "$tempfurnhotC * 1.8 + 32" | bc)
tempfurncoldF=$(echo "$tempfurncoldC * 1.8 + 32" | bc)

set -- $(tail $fndata | grep dhw | tail -1)
dtdhw="$1"
node="$2"
tempdhwhotC="$3"
tempdhwcoldC="$4"

tempdhwhotC=$(echo "$tempdhwhotC * $tempcpladj" | bc) 
tempdhwcoldC=$(echo "$tempdhwcoldC * $tempcpladj" | bc) 
tempdhwhotF=$(echo "$tempdhwhotC * 1.8 + 32" | bc)
tempdhwcoldF=$(echo "$tempdhwcoldC * 1.8 + 32" | bc)

echo "<html>" > $fnhtml
echo "<head>" >> $fnhtml
echo "<title>Furnance Monitor</title>" >> $fnhtml
echo "</head>" >> $fnhtml
echo "<body>" >> $fnhtml
echo "Time: $dtstr" >> $fnhtml
echo "<p>" >> $fnhtml
echo "Current Ambient Temperature: ${tempambF}F / ${tempambC}C" >> $fnhtml
echo "<p>" >> $fnhtml
echo "Current Furnace Supply Water Temperature: ${tempfurnhotF}F / ${tempfurnhotC}C" >> $fnhtml
echo "<p>" >> $fnhtml
echo "Current Furnace Return Water Temperature: ${tempfurncoldF}F / ${tempfurncoldC}C" >> $fnhtml
echo "<p>" >> $fnhtml
echo "Current Domestic Hot Water Supply Water Temperature: ${tempdhwhotF}F / ${tempdhwhotC}C" >> $fnhtml
echo "<p>" >> $fnhtml
echo "Current Domestic Hot Water Return Water Temperature: ${tempdhwcoldF}F / ${tempdhwcoldC}C" >> $fnhtml
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

dtstr=$(echo "$dtstr" | sed 's/\//\\\//g')
sed -e "s/FURNDT/$dtstr/g" -e "s/AMBTMP/$tempambF/g" -e "s/FNSUP/$tempfurnhotF/g" -e "s/FNRET/$tempfurncoldF/g" -e "s/DHWI/$tempdhwhotF/g" -e "s/DHWO/$tempdhwcoldF/g" $fnsyshtml > $webdir/$(basename $fnsyshtml)


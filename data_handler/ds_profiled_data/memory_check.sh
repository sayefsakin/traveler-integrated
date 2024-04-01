#!/bin/bash

declare -a ps_check=(	"ds_profiler.sh"
			"serve.py"
			"make"
			"cgal_data_server"
			"headless_test.py")
for i in "${!ps_check[@]}"
do
	if [ $i -ne 0 ]; then
		echo -n ",";
	fi
	printf "${ps_check[$i]}";
done
echo ""

for i in "${!ps_check[@]}"
do
   cps="${ps_check[$i]}"
   if [ $i -ne 0 ]; then
	   echo -n ",";
   fi
   if ps aux | grep -q "[${cps:0:1}]${cps:1}"
   then
	   cpsid=`ps aux | grep "[${cps:0:1}]${cps:1}" | awk '{printf $2}'`;
	   sudo pmap $cpsid | gawk '/total/ { a=strtonum($2); b=int(a/1024); printf b};'
   else
	   echo -n "0";
   fi
done
echo ""

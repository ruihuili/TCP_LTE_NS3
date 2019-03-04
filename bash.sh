#!/bin/bash    

n=0;
max=13;
while [ "$n" -le "$max" ]; do
	  mkdir "bashoutput/ctrlErrorOff/udpAM/location$n"
          ./waf --run "scratch/lte-rlcAm-ctrlErrorOff --positionIndex=\"$n\"  --simTime=50 --useUdp=1" >> bashoutput/ctrlErrorOff/averageThroughput_UdpAM.txt
          cp -r Dl* "bashoutput/ctrlErrorOff/udpAM/location$n"
          cp -r Ul* "bashoutput/ctrlErrorOff/udpAM/location$n"
          cp lte-grid.flowmon "bashoutput/ctrlErrorOff/udpAM/location$n" 
          n=`expr "$n" + 1`;
    done    
echo Hello World

#!/bin/bash    


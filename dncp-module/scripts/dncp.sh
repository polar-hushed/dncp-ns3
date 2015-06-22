#!/bin/sh

TOPOLOGY="0 1 2 3 4"
NNODE="5 10"
TRIALS="1 2 3 "

for topology in $TOPOLOGY
do
  for nnode in $NNODE
  do
    for trial in $TRIALS
    do
    echo Topology $topology, nNode $nnode, Trial $trial,
    ./waf --run "dncpex --topology=$topology --nNode=$nnode --trialID=$trial"
    done
  done
done

python parse_networkhash.py

mv  *.networkhash ./src/dncp/outputdata/rawdata
mv  *.packets ./src/dncp/outputdata/rawdata

mv  *.percentage ./src/dncp/outputdata/convergePercentage

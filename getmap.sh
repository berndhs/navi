#!/bin/bash -x

ARGC=$#
SOUTH=$1
WEST=$2
NORTH=$3
EAST=$4
if [ $ARGC > 4 ]
then
  OUTFILE=$5
else
  OUTFILE=map
fi
SERVER="http://api.openstreetmap.org/api/0.6/map"
wget ${SERVER}?bbox=${WEST},${SOUTH},${EAST},${NORTH} -O ${OUTFILE}.xml

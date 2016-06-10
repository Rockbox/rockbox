#!/bin/bash

./xmlconv $1 $2
./btcreate $2.wwt $2.wwr $2.wwi
rm $2.wwt $2.wwr

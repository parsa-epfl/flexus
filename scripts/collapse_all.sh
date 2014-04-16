#!/bin/bash

for FILE in */stats_db.out.gz ; do
  SEL=`echo $FILE | sed -e s/\.gz/\.selected\.gz/`
  stat-collapse 100000 149999 $FILE $SEL &
done
wait
stat-sample stats_db.out.gz */*.selected.gz

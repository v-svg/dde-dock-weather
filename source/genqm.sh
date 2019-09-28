#!/bin/bash
tsFiles=(`ls i18n/*.ts`)
for ts in "${tsFiles[@]}"
do
    printf "\nprocess ${ts}\n"
    lrelease "${ts}"
done

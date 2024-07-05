#! /bin/bash
# execute the hello 100 times, with a suffix of serial number

COUNT=0

while [ $COUNT -lt 100 ]; do
    let "COUNT++"
    echo "$(./hello)--${COUNT}"
done


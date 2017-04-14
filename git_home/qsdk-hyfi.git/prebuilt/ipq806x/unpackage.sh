#!/bin/sh
if [ -e "./etc" ]; 
then
    rm -rf ./etc
fi


if [ -e "./usr" ]; 
then
    rm -rf ./usr
fi


if [ -e "debian-binary" ]; 
then
    rm debian-binary
fi


if [ -e "data.tar.gz" ]; 
then
    rm data.tar.gz
fi


if [ -e "control.tar.gz" ]; 
then
    rm control.tar.gz
fi

tar xvf *.ipk
tar zxvf data.tar.gz

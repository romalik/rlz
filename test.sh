#!/bin/sh

./rlz c $1 $1.cmp $2
./rlz d $1.cmp $1.out

ls -la $1
ls -la $1.cmp

diff $1 $1.out


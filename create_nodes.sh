#!/bin/bash

function create { 
	for x in {0..9}; do 
		sudo mknod /dev/dummy_module${x} c `cat /proc/devices | egrep dummy | cut -d" " -f1` ${x};
		sudo chown harlock:harlock /dev/dummy_module${x}
	done
}
function delete {
	for x in {0..9}; do 
		sudo rm -drf /dev/dummy_module${x}
	done
}

if [ "$1" == "create" ]; then
	create;
fi

if [ "$1" == "delete" ]; then
	delete;
fi

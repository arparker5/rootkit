#/bin/bash
cd /opt/rootkit
make
insmod main.ko
rmmod main.ko

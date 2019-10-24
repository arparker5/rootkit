#/bin/bash
make                # compile module
insmod main.ko      # load module
rmmod main.ko       # unload module

#journalctl -xefk   # to see output

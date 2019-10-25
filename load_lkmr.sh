#/bin/bash
make                  # compile module
rmmod lkmr.ko
insmod lkmr.ko        # load module
#rmmod lkmr.ko        # unload module

#journalctl -xefk     # to see output

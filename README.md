# Linux Kernel Module Rootkit

## Andrew Parker, Alex Cater
##### CS 493 Project draft

For our project, we have decided to examine the inner workings of a rootkit to understand it,
infect a VM with it, and do some analysis on it. We want to look at methods of defending our VM
from the attacks of the rootkit.

For our rough draft, we have taken the code of a rootkit and done some analysis. While we went
through the code to get a better understanding, we made comments to explain the basics of how
our rootkit functions. Our goal for the rough draft is to have a good understanding of the
rootkit code. We have also infected an Ubuntu VM and messed around with some of the rootkit
functionalities.

### Functionality
* prints to kernel logs
* creates a device ``/dev/ttyR0``
  * has custom read/write/open/release functions
  * ``echo "CS493" > /dev/ttyR0`` gives root access to user
  * able to read string stored in kernel using a userspace program
    - at the moment only works on first read after loading the module. Trying to read again will return an empty string


### Useful links
##### how-tos
 * https://blog.trailofbits.com/2019/01/17/how-to-write-a-rootkit-without-really-trying/
 * http://tldp.org/LDP/lkmpg/2.6/html/lkmpg.html#AEN189

##### rootkit examples
* https://github.com/En14c/LilyOfTheValley
* https://0x00sec.org/t/kernel-rootkits-getting-your-hands-dirty/1485
* https://github.com/nurupo/rootkit

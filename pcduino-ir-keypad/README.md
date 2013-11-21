NOTE:

This is a sample gpio-based ir-keypad driver for pcDuino.
It was tested for my ir-remote controller only.
The driver is for NEC IR-controller only now, other devices you could change the code by yourself.
You have to changed something for your device to make it work:

1) make this driver work on pcDuino first, open terminal and run the following commands
$ sudo apt-get update && sudo apt-get install pcduino-linux-headers-3.4.29+
$ cd pcduino-ir-keypad
$ make M=`pwd` -C /usr/src/linux-3.4.29+
$ sudo insmod ir-keypad.ko pin=X 
( X is gpio number of pcDuino where your ir-receiver connected )

2) connect your ir-receiver to pcduino ( gpio0-4 or gpio7-17, default is gpio8)
$ sudo kmesg > /dev/null
$ sudo cat /proc/kmsg
then press the key one by one, the driver will print out raw code it received,
please write the rawcode you see to remote.h.
for example, when you press one key, maybe you will see the log like this:
# [ 3136.860000] code=f50abf40, key=65535(unknown)
write "0xf50abf40" to raw_codes in remote.h

after you have done this, update remote.h for your own remote controller.
a) MAX_KEYS is the key count of your remote controller
b) update keys and key_str to match raw_codes sequence

3) rebuild the driver 
$ make M=`pwd` -C /usr/src/linux-3.4.29+
$ sudo rmmod ir-keypad
$ sudo insmod ir-keypad.ko pin=X



PIC24-HID-Bootloader
====================
Allows you integrate `USB HID Bootloader` into your QT software.

Our project is based on [Microchip Libraries for Applications ](http://www.microchip.com/pagehandler/en-us/devtools/mla/) v2013-06-15. That package contains `HID Bootloader` demo project which located in following folder

    C:\MLA\USB\Device - Bootloaders\HID\software_cross_platform

We have refactored that project (split GUI and bootloader) to let your QT app operate with Microchip protocol using our shared classes. Also we extended bootloader with some useful functions like separate device read, write, verify, save to file, work with specified memory ranges etc.

Build project with QT Creator 5.2
---------------------------------
1. Remove all *.pro.user files from all project subfolders
2. Clear `/build` directory
3. Open PIC24-HID-Bootloader.pro in QT Creator
4. In project settings dialog leave only `Desktop Qt 5.*.* MinGW 32bit` checkbox and name building directories as 
 `PIC24-HID-Bootloader/build/Debug` and `PIC24-HID-Bootloader/build/Release`

Firmware
--------
We tested our project on [PIC24F Starter Kit](http://www.microchip.com/Developmenttools/ProductDetails.aspx?PartNO=DM240011) and custom PIC24FJ256GB106 device. Appropriate sources are attached in this repository. MLA package also contain sources for other MCU families. 

Testing the demo app with PIC24F Starter Kit
--------------------------------------------
1. Use MPLAB.X IDE to build [bootloader program](https://github.com/antonpinchuk/PIC24-HID-Bootloader/tree/master/Firmware/USB/Device%20-%20Bootloaders/HID/Firmware%20-%20PIC24FJ256GB106%20Family) and flash to PIC24F Starter Kit device
2. Turn analog potentiometer counter-clockwise and run app (LED should be white)
3. Run QT bootloader app, it should say it's disconnected.
4. Plug Starter Kit to PC via USB slave cable. LED should blink purple, app notify it's connected.
5. `Open` user_program.hex (download [here](https://github.com/antonpinchuk/PIC24-HID-Bootloader/blob/master/Firmware/User_Program_Demo.PIC24F_Starter_Kit.hex) or build from [sources](https://github.com/antonpinchuk/PIC24-HID-Bootloader/tree/master/Firmware/User%20Program%20Demo))
6. Click `Erase/Write/Verify`
7. Turn potentiometer clockwise and reset device, if LED blinks yellow means user program flashed successfully
8. Turn potentiometer back to bootloader mode (counter-clockwise) and reset device
9. Click `Read Device` and than `Save Memory Range` to binary file

TODO
----
1. Export memory dump to HEX format
2. Customize Microchip protocol with stream encryption (AES or XTEA)



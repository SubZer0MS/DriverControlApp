# DriverControlApp
GUI and CmdLine application that controls a driver + the driver

This Application can be used as an example of how to control (load/unload) a driver which has a GUI and CmdLine options as well :D

The driver (.sys) is included in the App (EXE) as a resource, so no need to copy that anywhere or anything like that.

The driver is included and it is a pretty good (start) example of how to write a driver. It currently can only allocate Paged or NonPaged Pool memory using different Pool Tags and allocation sizes.

Start it directly or run it with "/?" from a CMD to get the Usage information (available paramters).

I'll probably extend this more in the future, but this is the current functionality. Enjoy! :D

PS: You will need to set your machine in test mode unless you have a valid public official signed certificate: bcdedit /set testsigning on

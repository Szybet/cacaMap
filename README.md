# Inkbox maps
Maps app for inkbox os

https://user-images.githubusercontent.com/53944559/187004844-f8ebeec6-3ac7-439b-a314-a7aa6d7227ae.mp4

To run:
copy `tileservers.xml`, `loading.png` and `notavailable.jpeg` to the same dir as the binary, create a dir named `cache` and bind mount it to a tmpfs. It will limit itself to 30Mb, and clear at start / exit.

thanks to https://github.com/jmfairlie/cacaMap for the exelent backend 

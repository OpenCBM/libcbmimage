<!-- loosely based on https://github.com/othneildrew/Best-README-Template/blob/main/README.md?plain=1 -->

# libcbmimage

## About The Project

This is a library to access disk images that are common on the CBM/Commodore 8 bit computer series like the PET, the VIC-20, C64, C16, C116, Plus/4, C128 machines.

The disk images that can be processed are:

* 5,25":
    * D80 images of 8050 drives
    * D82 images of 8250 drives
    * D64 images of 4040/2031/1540/1541/1551/1570 drives
    * D71 images of 1571 drives
    * D40 images of early 2040 drives with Firmware V1

* 3,5":
    * D81 images of 1581 drives
    * D1M, D2M and D4M images of CMD FD2000 and CMD FD4000 drives

* Hard disks:
    * DHD support for CMD HDD images
    * D60 support for 9060 images
    * D90 support for 9090 images
    * D16 support for [Ruud Baltissen's special format](http://www.baltissen.org/newhtm/diskimag.htm)

## CAUTION

This is a prelimnary version of the library! Its API is still subject to change! It is only available for expository purposes.

## ROADMAP

- [x] Add D64, D40 support
- [ ] Add D64 support for 40, 41 and 42 track files
- [x] Add D71 support
- [x] Add D81 support
- [x] Add D80, D82 support
- [ ] Add D1M, D2M and D4M support (partially available)
- [ ] Add GEOS support (partially available)
- [ ] Add DHD support
- [ ] Add expoting into single-file formats (PRG, P64, S64, U64, L64,
- [ ] Add write support
- [ ] Abstract the reading interface so that the image does not have to exist as a PC file
- [ ] Create a stable API (v0.1.0)

## License

This library is distributed under the GPLv2 license. See `LICENSE.txt` for more information.


## Project homepage

[https://github.com/OpenCBM/libcbmimage](https://github.com/OpenCBM/libcbmimage)

## Contribution

* For problems, bug reports, feature requests, use the [issue tracker](https://github.com/OpenCBM/libcbmimage/issues)
* For contributions, create a [pull request](https://github.com/OpenCBM/libcbmimage/pulls)

## Additional links

* [Peter Scheper's file formats](https://ist.uwaterloo.ca/~schepers/formats.html)
* [Ruud Baltissen's file formats](http://www.baltissen.org/newhtm/diskimag.htm)
* [VICE emulator file formats](https://vice-emu.sourceforge.io/vice_17.html#SEC394)
* Florian Müller, Thorsten Petrowski: C64 GEOS 1.3. Markt & Technik 1988, ISBN 3-89090-570-6.

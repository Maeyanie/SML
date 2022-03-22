# SML
Format spec and reference converters for the SML 3D model format.

The SML format was created to help improve the situation with the growing size of STL files in the 3D printing community.

It combines the compact encoding of binary STLs with the list-and-reference of OBJs, adds a CRC32 for detecting corruption,
drops support for parts of the formats unimportant for 3D printing, and adds the option to use strips to further save space.
It is also extensible for future use.

With the reference converters, the created SML files are approximately 1/4 of the original size of the STL with zero loss in quality.


# Software Support
Aside from the reference converters, the following software supports SML:

PrusaSlicer via a forked version: https://github.com/Maeyanie/PrusaSlicer

Cura via a plugin: https://github.com/Maeyanie/SMLReaderPlugin

fsml (a fast SML viewer based on fstl): https://github.com/Maeyanie/fsml

SMLThumbnail (Windows Explorer SML thumbnails): https://github.com/Maeyanie/SMLThumbnails

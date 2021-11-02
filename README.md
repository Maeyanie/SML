# SML
Format spec and reference converters for the SML 3D model format.

The SML format was created to help improve the situation with the growing size of STL files in the 3D printing community.

It combines the compact encoding of binary STLs with the list-and-reference of OBJs, adds a CRC32 for detecting corruption,
drops support for parts of the formats unimportant for 3D printing, and adds the option to use strips to further save space.
It is also extensible for future use.

With the reference converters, the created SML files are approximately 1/3 of the original size of the STL with zero loss in quality;
and they aren't even using the more advanced space-saving features of the format yet.

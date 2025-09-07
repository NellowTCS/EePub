# EePub
ePub rendering for embedded and/or resource constrained devices.

You need to build Lexbor first:

```shell
cd external/lexbor
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF
make
```

Then build the library.

```shell
cd ../../../
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF
make
```
### libgenpattern [WIP]
##### Checklist
- [x] Convex hull extraction algorithm
  - [x] Multithreading
  - [x] AVX support
- [ ] Polygon translation
  - [ ] AVX support
- [ ] Shoelace formula
  - [ ] AVX support
- [ ] Polygon intersection 
  - [ ] Line segment intersection point
- [ ] Polygon distance
  - [ ] Point distance
- [ ] Pattern search

<i>to be continued...</i>
##### Build
```
mkdir build
cd build
CFLAGS=-mavx2 meson --buildtype=release ..
ninja all
```
<i>Note:</i> Run without `CFLAGS=-mavx2` if target CPU doesn't support AVX2.
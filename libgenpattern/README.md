### libgenpattern [WIP]
##### Checklist
- [x] Multithreading
- [x] Convex hull extraction algorithm
  - [x] AVX support
- [x] Polygon translation
  - [x] AVX support
- [x] Shoelace formula
  - [x] AVX support
- [ ] Polygon offsetting
  - [ ] AVX support?
- [ ] Polygon intersection 
  - [ ] Line segment intersection point
- [ ] Polygon distance
  - [ ] Point distance
  - [ ] AVX support?
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
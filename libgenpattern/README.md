### libgenpattern [WIP]
##### Checklist
- [x] Multithreading
- [x] Convex hull extraction algorithm
  - [x] AVX support
- [x] Polygon translation
  - [x] AVX support
- [x] Trapezoid formula
  - [x] AVX support
- [ ] Polygon intersection 
  - [ ] Line segment intersection point
- [ ] Polygon distance
  - ...
  - [ ] AVX support?
- [ ] Pattern search

<i>to be continued...</i>
##### Build
```
mkdir build
cd build
CFLAGS="-mavx2 -mfma" meson --buildtype=release ..
ninja all
```
<i>Note:</i> Run without `CFLAGS="-mavx2 -mfma"` if target CPU doesn't support AVX2 or FMA.
### libgenpattern [WIP]
##### Checklist
- [x] Multithreading
- [x] Convex hull extraction algorithm
  - [x] AVX support
- [x] Polygon translation
- [x] Trapezoid formula
- [x] Polygon intersection 
- [x] Polygon distance (Edelsbrunner's algorithm)
- [ ] Pattern search
- [ ] R-Trees?

<i>to be continued...</i>
##### Build
```
mkdir build
cd build
CFLAGS="-mavx2 -mfma" meson --buildtype=release ..
ninja all
```
<i>Note:</i> Run without `CFLAGS="-mavx2 -mfma"` if target CPU doesn't support AVX2 or FMA.
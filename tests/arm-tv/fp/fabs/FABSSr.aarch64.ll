define float @absf(float %0) {
  %2 = bitcast float %0 to i32
  %3 = and i32 %2, 2147483647
  %4 = bitcast i32 %3 to float
  ret float %4
}

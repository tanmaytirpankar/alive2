define i1 @fcmpRI_X_one(float %0) {
  %2 = fcmp one float %0, 0.000000e+00
  ret i1 %2
}

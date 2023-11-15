define i1 @fcmp_float_une(float %a, float %b) {
   %c = fcmp une float %a, %b
   ret i1 %c
}

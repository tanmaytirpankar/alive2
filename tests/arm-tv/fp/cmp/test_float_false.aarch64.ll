define i1 @fcmp_float_false(float %a, float %b) {
   %c = fcmp false float %a, %b
   ret i1 %c
}

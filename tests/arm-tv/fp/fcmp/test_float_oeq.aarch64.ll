define i1 @fcmp_float_oeq(float %a, float %b) {
   %c = fcmp oeq float %a, %b
   ret i1 %c
}

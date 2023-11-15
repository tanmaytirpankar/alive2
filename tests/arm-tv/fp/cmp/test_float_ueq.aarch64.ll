define i1 @fcmp_float_ueq(float %a, float %b) {
   %c = fcmp ueq float %a, %b
   ret i1 %c
}

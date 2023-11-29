define i1 @fcmp_float_olt(float %a, float %b) {
   %c = fcmp olt float %a, %b
   ret i1 %c
}

define i1 @fcmp_float_ult(float %a, float %b) {
   %c = fcmp ult float %a, %b
   ret i1 %c
}

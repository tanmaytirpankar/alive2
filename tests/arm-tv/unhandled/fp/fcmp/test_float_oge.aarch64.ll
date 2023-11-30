define i1 @fcmp_float_oge(float %a, float %b) {
   %c = fcmp oge float %a, %b
   ret i1 %c
}

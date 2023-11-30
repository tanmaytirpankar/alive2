define i1 @fcmp_float_uge(float %a, float %b) {
   %c = fcmp uge float %a, %b
   ret i1 %c
}

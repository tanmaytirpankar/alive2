define i1 @fcmp_float_ugt(float %a, float %b) {
   %c = fcmp ugt float %a, %b
   ret i1 %c
}

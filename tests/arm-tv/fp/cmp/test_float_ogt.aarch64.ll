define i1 @fcmp_float_ogt(float %a, float %b) {
   %c = fcmp ogt float %a, %b
   ret i1 %c
}

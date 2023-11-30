define i1 @fcmp_float_true(float %a, float %b) {
   %c = fcmp true float %a, %b
   ret i1 %c
}

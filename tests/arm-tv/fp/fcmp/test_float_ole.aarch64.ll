define i1 @fcmp_float_ole(float %a, float %b) {
   %c = fcmp ole float %a, %b
   ret i1 %c
}

define i1 @fcmp_float_uno(float %a, float %b) {
   %c = fcmp uno float %a, %b
   ret i1 %c
}

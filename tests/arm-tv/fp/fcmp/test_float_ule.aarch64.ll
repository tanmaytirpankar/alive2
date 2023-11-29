define i1 @fcmp_float_ule(float %a, float %b) {
   %c = fcmp ule float %a, %b
   ret i1 %c
}

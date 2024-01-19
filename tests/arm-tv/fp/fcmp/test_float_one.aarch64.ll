define i1 @fcmp_float_one(float %a, float %b) {
   %c = fcmp one float %a, %b
   ret i1 %c
}

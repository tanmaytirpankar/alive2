define i1 @fcmp_float_ord(float %a, float %b) {
   %c = fcmp ord float %a, %b
   ret i1 %c
}

define i1 @fcmp_double_une(double %a, double %b) {
   %c = fcmp une double %a, %b
   ret i1 %c
}

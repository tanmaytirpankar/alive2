define i1 @fcmp_double_ule(double %a, double %b) {
   %c = fcmp ule double %a, %b
   ret i1 %c
}

define i1 @fcmp_double_ult(double %a, double %b) {
   %c = fcmp ult double %a, %b
   ret i1 %c
}

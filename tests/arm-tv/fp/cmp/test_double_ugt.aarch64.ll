define i1 @fcmp_double_ugt(double %a, double %b) {
   %c = fcmp ugt double %a, %b
   ret i1 %c
}

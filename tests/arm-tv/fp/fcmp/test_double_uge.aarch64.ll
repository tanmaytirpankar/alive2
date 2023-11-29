define i1 @fcmp_double_uge(double %a, double %b) {
   %c = fcmp uge double %a, %b
   ret i1 %c
}

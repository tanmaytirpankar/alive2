define i1 @fcmp_double_olt(double %a, double %b) {
   %c = fcmp olt double %a, %b
   ret i1 %c
}

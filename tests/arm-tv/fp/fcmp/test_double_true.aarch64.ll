define i1 @fcmp_double_true(double %a, double %b) {
   %c = fcmp true double %a, %b
   ret i1 %c
}

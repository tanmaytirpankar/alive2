define i1 @fcmp_double_false(double %a, double %b) {
   %c = fcmp false double %a, %b
   ret i1 %c
}

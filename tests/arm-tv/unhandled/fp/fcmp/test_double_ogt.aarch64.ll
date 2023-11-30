define i1 @fcmp_double_ogt(double %a, double %b) {
   %c = fcmp ogt double %a, %b
   ret i1 %c
}

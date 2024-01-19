define i1 @fcmp_double_oeq(double %a, double %b) {
   %c = fcmp oeq double %a, %b
   ret i1 %c
}

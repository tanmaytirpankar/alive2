define i1 @fcmp_double_ole(double %a, double %b) {
   %c = fcmp ole double %a, %b
   ret i1 %c
}

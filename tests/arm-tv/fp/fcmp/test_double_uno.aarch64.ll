define i1 @fcmp_double_uno(double %a, double %b) {
   %c = fcmp uno double %a, %b
   ret i1 %c
}

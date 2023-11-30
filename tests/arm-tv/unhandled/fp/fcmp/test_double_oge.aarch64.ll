define i1 @fcmp_double_oge(double %a, double %b) {
   %c = fcmp oge double %a, %b
   ret i1 %c
}

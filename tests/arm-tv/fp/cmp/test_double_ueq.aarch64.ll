define i1 @fcmp_double_ueq(double %a, double %b) {
   %c = fcmp ueq double %a, %b
   ret i1 %c
}

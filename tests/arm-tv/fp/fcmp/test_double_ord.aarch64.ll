define i1 @fcmp_double_ord(double %a, double %b) {
   %c = fcmp ord double %a, %b
   ret i1 %c
}

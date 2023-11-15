define i1 @fcmp_double_one(double %a, double %b) {
   %c = fcmp one double %a, %b
   ret i1 %c
}

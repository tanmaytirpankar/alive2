define double @fadd (double %a) {
  %y = fadd double 0.25, %a
  ret double %y
}

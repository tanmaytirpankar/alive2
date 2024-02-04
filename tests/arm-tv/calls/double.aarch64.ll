declare double @g(double)

define double @f(double) {
  %a = fadd double %0, 1.0
  %b = call double @g(double %a)
  %c = fadd double %b, 2.0
  ret double %c
}

declare float @g(float)

define float @f(float) {
  %a = fadd float %0, 1.0
  %b = call float @g(float %a)
  %c = fadd float %b, 2.0
  ret float %c
}

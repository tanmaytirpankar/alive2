declare float @g(i1, i2, float, i64)

define float @f(float) {
  %a = fadd float %0, 1.0
  %b = call float @g(i1 0, i2 3, float %a, i64 -1)
  %c = fadd float %b, 2.0
  ret float %c
}

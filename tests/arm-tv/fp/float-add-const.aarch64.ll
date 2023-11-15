define float @fadd (float %a) {
  %y = fadd float %a, -2.0
  ret float %y
}

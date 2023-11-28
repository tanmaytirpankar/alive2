define float @f(i1 %0, float %1, float %2) {
  %4 = select i1 %0, float %2, float %1
  ret float %4
}

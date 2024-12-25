declare float @llvm.rint.f32(float) #0

define float @f(float %0) {
  %2 = call float @llvm.rint.f32(float %0)
  ret float %2
}

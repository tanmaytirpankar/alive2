declare void @llvm.assume(i1 noundef) #0

define i1 @assume_ule_neg1__oeq_0(float %0) {
  %2 = fcmp ule float %0, -1.000000e+00
  call void @llvm.assume(i1 %2)
  %3 = fcmp oeq float %0, 0.000000e+00
  ret i1 %3
}

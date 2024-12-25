; ModuleID = '<stdin>'
source_filename = "<stdin>"

define i1 @f(double %0) {
  %2 = fadd nnan double %0, 1.000000e+00
  %3 = call double @llvm.rint.f64(double %2)
  %4 = fcmp uno double %3, %3
  ret i1 %4
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare double @llvm.rint.f64(double) #0



; ModuleID = '<stdin>'
source_filename = "<stdin>"

; Function Attrs: nounwind ssp strictfp uwtable(sync)
define float @f(float noundef %0, float noundef %1) {
  %3 = alloca float, align 4
  %4 = alloca float, align 4
  store float %0, ptr %3, align 4
  store float %1, ptr %4, align 4
  ret float 0.000000e+00
}



!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 2, !"SDK Version", [2 x i32] [i32 14, i32 2]}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 8, !"PIC Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 1}
!5 = !{!"clang version 18.0.0git (git@github.com:llvm/llvm-project.git e27561fc7de0231f2efdb750f2092c3ac807c1a3)"}

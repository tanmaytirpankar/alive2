; ModuleID = 'test-073834435.ll'
source_filename = "<stdin>"

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: readwrite)
define void @f(ptr nocapture noundef writeonly %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 {
  %3 = getelementptr inbounds float, ptr %1, i64 1
  %4 = load float, ptr %3, align 4
  %5 = fneg float %4
  store float %5, ptr %0, align 4
  %6 = load float, ptr %1, align 4
  %7 = getelementptr inbounds float, ptr %0, i64 1
  store float %6, ptr %7, align 4
  ret void
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: readwrite) }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 2, !"SDK Version", [2 x i32] [i32 14, i32 2]}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 8, !"PIC Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 1}
!5 = !{!"clang version 18.0.0git (git@github.com:llvm/llvm-project.git e27561fc7de0231f2efdb750f2092c3ac807c1a3)"}

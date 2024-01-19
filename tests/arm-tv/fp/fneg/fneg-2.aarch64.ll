; ModuleID = 'test-141320000.ll'
source_filename = "<stdin>"

%0 = type { ptr, ptr, ptr, ptr, i32, i32, i32, i32, float, [3 x float], float, [3 x float], [3 x float], [3 x float], [3 x float], [3 x float], [4 x [4 x float]], [4 x [4 x float]], [4 x [4 x float]], float, float, float, float, float, float, float, float, float, [3 x float], float, float, float, i32, i32, ptr, [4 x [4 x float]], [4 x [4 x float]], [4 x [4 x float]], [4 x [4 x float]], ptr, ptr, ptr, ptr, ptr, %1 }
%1 = type { ptr, ptr }

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: readwrite)
define void @f(ptr nocapture noundef %0, float noundef %1, float noundef %2, float noundef %3, float noundef %4) local_unnamed_addr #0 {
  %6 = getelementptr inbounds %0, ptr %0, i64 0, i32 10
  store float %4, ptr %6, align 8
  %7 = getelementptr inbounds %0, ptr %0, i64 0, i32 5
  %8 = load i32, ptr %7, align 4
  %9 = and i32 %8, 16
  %.not = icmp eq i32 %9, 0
  br i1 %.not, label %12, label %10

10:                                               ; preds = %5
  %11 = fneg float %4
  store float %11, ptr %6, align 8
  br label %12

12:                                               ; preds = %10, %5
  %13 = getelementptr inbounds %0, ptr %0, i64 0, i32 11
  store float %1, ptr %13, align 4
  %14 = getelementptr inbounds %0, ptr %0, i64 0, i32 11, i64 1
  store float %2, ptr %14, align 4
  %15 = getelementptr inbounds %0, ptr %0, i64 0, i32 11, i64 2
  store float %3, ptr %15, align 4
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

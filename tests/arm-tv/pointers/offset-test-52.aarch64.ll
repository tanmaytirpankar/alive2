; ModuleID = 'foo.c'
source_filename = "foo.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx13.0.0"

%struct.s = type { i16, i32, i8, i8, i8, i8, i8, i8 }

; Function Attrs: mustprogress nofree norecurse nosync nounwind ssp uwtable willreturn
define void @f(%struct.s* nocapture %0) local_unnamed_addr #0 {
  %2 = getelementptr inbounds %struct.s, %struct.s* %0, i64 0, i32 6
  %3 = load i8, i8* %2, align 4
  %4 = getelementptr inbounds %struct.s, %struct.s* %0, i64 0, i32 1
  %5 = load i32, i32* %4, align 4
  %6 = getelementptr inbounds %struct.s, %struct.s* %0, i64 0, i32 2
  %7 = load i8, i8* %6, align 4
  %8 = trunc i32 %5 to i8
  %9 = shl i8 %8, 1
  %10 = add i8 %7, %3
  %11 = add i8 %10, %9
  %12 = getelementptr inbounds %struct.s, %struct.s* %0, i64 0, i32 3
  store i8 %11, i8* %12, align 1
  %13 = getelementptr inbounds %struct.s, %struct.s* %0, i64 0, i32 4
  store i8 %11, i8* %13, align 2
  store i8 %11, i8* %2, align 4
  %14 = getelementptr inbounds %struct.s, %struct.s* %0, i64 0, i32 7
  %15 = load i8, i8* %14, align 1
  %16 = shl i8 %15, 1
  %17 = add i8 %11, %7
  %18 = add i8 %17, %16
  %19 = getelementptr inbounds %struct.s, %struct.s* %0, i64 0, i32 5
  store i8 %18, i8* %19, align 1
  ret void
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind ssp uwtable willreturn "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+crc,+crypto,+dotprod,+fp-armv8,+fp16fml,+fullfp16,+lse,+neon,+ras,+rcpc,+rdm,+sha2,+sha3,+sm4,+v8.5a,+zcm,+zcz" }

!llvm.module.flags = !{!0, !1, !2, !3, !4, !5, !6, !7, !8}
!llvm.ident = !{!9}

!0 = !{i32 2, !"SDK Version", [2 x i32] [i32 13, i32 1]}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 1, !"branch-target-enforcement", i32 0}
!3 = !{i32 1, !"sign-return-address", i32 0}
!4 = !{i32 1, !"sign-return-address-all", i32 0}
!5 = !{i32 1, !"sign-return-address-with-bkey", i32 0}
!6 = !{i32 7, !"PIC Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 1}
!8 = !{i32 7, !"frame-pointer", i32 1}
!9 = !{!"Apple clang version 14.0.0 (clang-1400.0.29.202)"}
; ModuleID = 'foo.c'
source_filename = "foo.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx13.0.0"

%struct.s = type { i64, i8, i16, i8, i32, i16, i16, i8 }

; Function Attrs: mustprogress nofree norecurse nosync nounwind ssp uwtable willreturn
define void @f(%struct.s* nocapture %0) local_unnamed_addr #0 {
  %2 = getelementptr inbounds %struct.s, %struct.s* %0, i64 0, i32 4
  %3 = load i32, i32* %2, align 8
  %4 = getelementptr inbounds %struct.s, %struct.s* %0, i64 0, i32 1
  %5 = load i8, i8* %4, align 8
  %6 = zext i8 %5 to i32
  %7 = add nsw i32 %3, %6
  %8 = trunc i32 %7 to i16
  %9 = getelementptr inbounds %struct.s, %struct.s* %0, i64 0, i32 2
  store i16 %8, i16* %9, align 2
  %10 = getelementptr inbounds %struct.s, %struct.s* %0, i64 0, i32 0
  %11 = load i64, i64* %10, align 8
  %12 = trunc i64 %11 to i32
  %13 = add i32 %7, %12
  %14 = trunc i32 %13 to i8
  %15 = getelementptr inbounds %struct.s, %struct.s* %0, i64 0, i32 3
  store i8 %14, i8* %15, align 4
  store i32 %13, i32* %2, align 8
  %16 = sext i32 %13 to i64
  store i64 %16, i64* %10, align 8
  %17 = and i32 %7, 65535
  %18 = add nsw i32 %13, %17
  %19 = shl i32 %13, 24
  %20 = ashr exact i32 %19, 24
  %21 = add nsw i32 %18, %20
  %22 = trunc i32 %21 to i16
  store i16 %22, i16* %9, align 2
  %23 = sext i32 %21 to i64
  store i64 %23, i64* %10, align 8
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
; ModuleID = 'function_switch.c'
source_filename = "function_switch.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

; Function Attrs: mustprogress nofree norecurse nosync nounwind ssp willreturn memory(none) uwtable(sync)
define signext i8 @foo(ptr noundef readnone %0) #0 {
  %2 = icmp eq ptr %0, @f1
  br i1 %2, label %14, label %3

3:                                                ; preds = %1
  %4 = icmp eq ptr %0, @f2
  br i1 %4, label %14, label %5

5:                                                ; preds = %3
  %6 = icmp eq ptr %0, @f3
  br i1 %6, label %14, label %7

7:                                                ; preds = %5
  %8 = icmp eq ptr %0, @f4
  br i1 %8, label %14, label %9

9:                                                ; preds = %7
  %10 = icmp eq ptr %0, @f5
  br i1 %10, label %14, label %11

11:                                               ; preds = %9
  %12 = icmp eq ptr %0, @f6
  %13 = select i1 %12, i8 6, i8 0
  br label %14

14:                                               ; preds = %11, %9, %7, %5, %3, %1
  %15 = phi i8 [ 1, %1 ], [ 2, %3 ], [ 3, %5 ], [ 4, %7 ], [ 5, %9 ], [ %13, %11 ]
  ret i8 %15
}

declare signext i8 @f1(ptr noundef) #1

declare signext i8 @f2(ptr noundef) #1

declare signext i8 @f3(ptr noundef) #1

declare signext i8 @f4(ptr noundef) #1

declare signext i8 @f5(ptr noundef) #1

declare signext i8 @f6(ptr noundef) #1

attributes #0 = { mustprogress nofree norecurse nosync nounwind ssp willreturn memory(none) uwtable(sync) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+crc,+dotprod,+fp-armv8,+fp16fml,+fullfp16,+lse,+neon,+ras,+rcpc,+rdm,+sha2,+sha3,+v8.1a,+v8.2a,+v8.3a,+v8.4a,+v8.5a,+v8a,+zcm,+zcz" }
attributes #1 = { "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+crc,+dotprod,+fp-armv8,+fp16fml,+fullfp16,+lse,+neon,+ras,+rcpc,+rdm,+sha2,+sha3,+v8.1a,+v8.2a,+v8.3a,+v8.4a,+v8.5a,+v8a,+zcm,+zcz" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 2, !"SDK Version", [2 x i32] [i32 14, i32 2]}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 8, !"PIC Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 1}
!5 = !{!"clang version 17.0.3"}

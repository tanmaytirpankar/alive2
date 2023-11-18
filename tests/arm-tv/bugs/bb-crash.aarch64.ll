source_filename = "/home/regehr/arm-tests/test-340089215.ll"

define i1 @test_max_row_limit(i32 %0, i32 %1, i32 %2, i32 %3, i32 %4) {
  %6 = trunc i32 %3 to i1
  %7 = icmp uge i32 %0, 100
  br i1 %7, label %8, label %19

8:                                                ; preds = %5
  %9 = icmp uge i32 %1, 100
  br i1 %9, label %10, label %19

10:                                               ; preds = %8
  %11 = icmp uge i32 %2, 100
  br i1 %11, label %12, label %19

12:                                               ; preds = %10
  %13 = icmp uge i32 %3, 100
  br i1 %13, label %14, label %19

14:                                               ; preds = %12
  %15 = icmp uge i32 %4, 100
  br i1 %15, label %16, label %19

16:                                               ; preds = %14
  %17 = icmp uge i32 %4, 100
  %18 = call i1 @llvm.ushl.sat.i1(i1 %6, i1 true)
  ret i1 %18

19:                                               ; preds = %14, %12, %10, %8, %5
  ret i1 false
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i1 @llvm.ushl.sat.i1(i1, i1) #0


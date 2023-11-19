define i32 @test_05_select(i32 %0, i32 %1, i1 %2, i1 %3) {
  %5 = icmp slt i32 %0, %1
  %6 = select i1 %5, i1 true, i1 %2
  %7 = select i1 %6, i1 true, i1 %3
  br i1 %7, label %8, label %11

8:                                                ; preds = %4
  %9 = icmp slt i32 %0, %1
  %10 = select i1 %9, i32 %0, i32 %1
  ret i32 %10

11:                                               ; preds = %4
  %12 = icmp slt i32 %0, %1
  %13 = udiv i1 %3, false
  %14 = select i1 %13, i32 %0, i32 %1
  ret i32 %14
}

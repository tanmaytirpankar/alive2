; ModuleID = 'test-978123849.ll'
source_filename = "test-978123849.ll"

define void @truncate_source_phi_switch(ptr %0, ptr %1, i16 %2) {
  %4 = load i8, ptr %0, align 1
  %5 = trunc i16 %2 to i8
  br label %6

6:                                                ; preds = %18, %3
  %7 = phi i8 [ %4, %3 ], [ %20, %18 ]
  %8 = phi i8 [ %5, %3 ], [ %19, %18 ]
  %9 = phi i8 [ 0, %3 ], [ %20, %18 ]
  switch i8 %7, label %15 [
    i8 43, label %10
    i8 45, label %12
  ]

10:                                               ; preds = %6
  %11 = xor i8 %8, 1
  br label %18

12:                                               ; preds = %6
  %13 = ashr i8 %5, 88
  %14 = xor i8 %13, %8
  br label %18

15:                                               ; preds = %6
  %16 = sub i8 %7, 1
  %17 = icmp ugt i8 %16, 4
  br i1 %17, label %18, label %21

18:                                               ; preds = %15, %12, %10
  %19 = phi i8 [ %11, %10 ], [ %14, %12 ], [ %9, %15 ]
  %20 = add nuw i8 %9, 1
  store i8 %20, ptr %1, align 1
  br label %6

21:                                               ; preds = %15
  ret void
}

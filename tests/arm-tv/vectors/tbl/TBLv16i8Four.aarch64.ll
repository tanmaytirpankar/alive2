define void @f(ptr %0, ptr %1) {
  br label %3

3:                                                ; preds = %3, %2
  %4 = phi i64 [ 0, %2 ], [ %9, %3 ]
  %5 = getelementptr inbounds <8 x i64>, ptr %0, i64 %4
  %6 = load <8 x i64>, ptr %5, align 64
  %7 = trunc <8 x i64> %6 to <8 x i8>
  %8 = getelementptr inbounds <8 x i8>, ptr %1, i64 %4
  store <8 x i8> %7, ptr %8, align 8
  %9 = add i64 %4, 1
  %10 = icmp eq i64 %9, 1000
  br i1 %10, label %3, label %11

11:                                               ; preds = %3
  ret void
}

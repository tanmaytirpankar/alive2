define void @f(i64 %0, ptr %1) {
  %3 = trunc i64 %0 to i32
  %4 = icmp eq i32 %3, 0
  br i1 %4, label %6, label %5

5:                                                ; preds = %2
  store i64 0, ptr %1, align 8
  br label %6

6:                                                ; preds = %5, %2
  ret void
}

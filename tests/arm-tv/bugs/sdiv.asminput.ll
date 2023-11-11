define i32 @f3(i32 %0, i32 %1, i1 %2) {
  br i1 %2, label %6, label %4

4:                                                ; preds = %3
  %5 = icmp eq i32 %0, 0
  %. = select i1 %5, i32 2, i32 0
  br label %common.ret

common.ret:                                       ; preds = %6, %4
  %common.ret.op = phi i32 [ %7, %6 ], [ %., %4 ]
  ret i32 %common.ret.op

6:                                                ; preds = %3
  %7 = sdiv i32 %1, %0
  br label %common.ret
}

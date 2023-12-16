define void @sink_v2z64_1(ptr %0, ptr %1, i64 %2, <2 x i32> %3) {
  %5 = zext <2 x i32> %3 to <2 x i64>
  %6 = shufflevector <2 x i64> %5, <2 x i64> poison, <2 x i32> <i32 1, i32 1>
  br label %7

7:                                                ; preds = %7, %4
  %8 = phi i64 [ 0, %4 ], [ %18, %7 ]
  %9 = getelementptr inbounds i32, ptr %0, i64 %8
  %10 = bitcast ptr %9 to ptr
  %11 = load <2 x i32>, ptr %10, align 4
  %12 = zext <2 x i32> %11 to <2 x i64>
  %13 = mul <2 x i64> %12, %6
  %14 = ashr <2 x i64> %13, <i64 15, i64 15>
  %15 = trunc <2 x i64> %14 to <2 x i32>
  %16 = getelementptr inbounds i32, ptr %1, i64 %8
  %17 = bitcast ptr %9 to ptr
  store <2 x i32> %15, ptr %17, align 4
  %18 = add nuw i64 %8, 8
  %19 = icmp eq i64 %18, %2
  br i1 %19, label %20, label %7

20:                                               ; preds = %7
  ret void
}

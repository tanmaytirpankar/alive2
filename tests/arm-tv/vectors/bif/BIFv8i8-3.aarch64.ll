define <2 x i32> @test30vec(i1 %0) {
  br i1 %0, label %3, label %2

2:                                                ; preds = %1
  br label %3

3:                                                ; preds = %2, %1
  %4 = phi <2 x i32> [ <i32 1000, i32 1000>, %1 ], [ <i32 10, i32 10>, %2 ]
  %5 = xor <2 x i32> %4, <i32 123, i32 123>
  ret <2 x i32> %5
}

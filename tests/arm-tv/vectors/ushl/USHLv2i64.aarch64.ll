define <2 x i64> @test3(<2 x i64> %0, <2 x i64> %1) {
  %3 = shufflevector <2 x i64> %1, <2 x i64> undef, <2 x i32> zeroinitializer
  %4 = shl <2 x i64> %0, %3
  ret <2 x i64> %4
}

define <4 x i32> @test9(<4 x i32> %0) {
  %2 = and <4 x i32> %0, <i32 -1, i32 -1, i32 0, i32 0>
  ret <4 x i32> %2
}

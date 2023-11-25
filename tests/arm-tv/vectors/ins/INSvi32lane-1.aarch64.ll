define <4 x i32> @test4(<4 x i32> %0) {
  %2 = and <4 x i32> %0, <i32 0, i32 0, i32 0, i32 -1>
  ret <4 x i32> %2
}

define <4 x i32> @test8(<4 x i32> %0) {
  %2 = sdiv <4 x i32> %0, <i32 7, i32 7, i32 7, i32 7>
  ret <4 x i32> %2
}

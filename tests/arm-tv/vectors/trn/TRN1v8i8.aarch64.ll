define <8 x i8> @test_vtrn1_s8(<8 x i8> %0, <8 x i8> %1) {
  %3 = shufflevector <8 x i8> %0, <8 x i8> %1, <8 x i32> <i32 0, i32 8, i32 2, i32 10, i32 4, i32 12, i32 6, i32 14>
  ret <8 x i8> %3
}

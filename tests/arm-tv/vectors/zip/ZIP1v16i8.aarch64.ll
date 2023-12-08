define <16 x i8> @test_same_vzip1q_p8(<16 x i8> %0) {
  %2 = shufflevector <16 x i8> %0, <16 x i8> %0, <16 x i32> <i32 0, i32 16, i32 1, i32 17, i32 2, i32 18, i32 3, i32 19, i32 4, i32 20, i32 5, i32 21, i32 6, i32 22, i32 7, i32 23>
  ret <16 x i8> %2
}

define <16 x i8> @test_div7_16i8(<16 x i8> %0) {
  %2 = udiv <16 x i8> %0, <i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7>
  ret <16 x i8> %2
}

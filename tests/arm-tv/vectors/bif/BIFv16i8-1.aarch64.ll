define <16 x i8> @test8(<16 x i8> %0, <16 x i8> %1, i1 %2) {
  %4 = select i1 %2, <16 x i8> %0, <16 x i8> %1
  ret <16 x i8> %4
}

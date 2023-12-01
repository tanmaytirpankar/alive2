define <2 x i32> @test1(i1 %0, <2 x i32> %1, <2 x i32> %2) {
  %4 = select i1 %0, <2 x i32> %1, <2 x i32> %2
  ret <2 x i32> %4
}

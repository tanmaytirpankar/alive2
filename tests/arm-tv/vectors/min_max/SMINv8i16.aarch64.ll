define i16 @test_reduce_v8i16(<8 x i16> %0) {
  %2 = shufflevector <8 x i16> %0, <8 x i16> undef, <8 x i32> <i32 4, i32 5, i32 6, i32 7, i32 poison, i32 poison, i32 poison, i32 poison>
  %3 = icmp slt <8 x i16> %0, %2
  %4 = select <8 x i1> %3, <8 x i16> %0, <8 x i16> %2
  %5 = shufflevector <8 x i16> %4, <8 x i16> undef, <8 x i32> <i32 2, i32 3, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
  %6 = icmp slt <8 x i16> %4, %5
  %7 = select <8 x i1> %6, <8 x i16> %4, <8 x i16> %5
  %8 = shufflevector <8 x i16> %7, <8 x i16> undef, <8 x i32> <i32 1, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
  %9 = icmp slt <8 x i16> %7, %8
  %10 = select <8 x i1> %9, <8 x i16> %7, <8 x i16> %8
  %11 = extractelement <8 x i16> %10, i32 0
  ret i16 %11
}

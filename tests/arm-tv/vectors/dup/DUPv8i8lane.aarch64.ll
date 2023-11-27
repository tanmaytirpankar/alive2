define <8 x i8> @splat_ve4(<8 x i8> %0) {
  %2 = shufflevector <8 x i8> %0, <8 x i8> poison, <8 x i32> <i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4>
  ret <8 x i8> %2
}

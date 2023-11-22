define <4 x i32> @spltRegVali(i32 signext %0) {
  %2 = insertelement <4 x i32> undef, i32 %0, i32 0
  %3 = shufflevector <4 x i32> %2, <4 x i32> undef, <4 x i32> zeroinitializer
  ret <4 x i32> %3
}

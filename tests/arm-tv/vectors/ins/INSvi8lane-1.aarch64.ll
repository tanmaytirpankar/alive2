define <8 x i8> @ins8b8(<8 x i8> %0, <8 x i8> %1) {
  %3 = extractelement <8 x i8> %0, i32 2
  %4 = insertelement <8 x i8> %1, i8 %3, i32 4
  ret <8 x i8> %4
}

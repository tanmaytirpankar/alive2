define <8 x i8> @shl_v8i8(<8 x i8> %0, <8 x i8> %1) {
  %3 = shl <8 x i8> %0, %1
  ret <8 x i8> %3
}

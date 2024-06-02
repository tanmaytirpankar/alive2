declare <8 x i8> @g(<8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>)

define <8 x i8> @f() {
  %x = call <8 x i8> @g(<8 x i8> 0, <8 x i8> 1, <8 x i8> 2, <8 x i8> 3, <8 x i8> 4, <8 x i8> 5, <8 x i8> 6, <8 x i8> 7, <8 x i8> 8, <8 x i8> 9, <8 x i8> 10, <8 x i8> 11, <8 x i8> 12, <8 x i8> 13, <8 x i8> 14)
  ret <8 x i8> %x
}

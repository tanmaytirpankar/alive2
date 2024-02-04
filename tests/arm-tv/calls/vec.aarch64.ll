declare <8 x i8> @g(<8 x i8>)

define <8 x i8> @f(<8 x i8>) {
  %a = add <8 x i8> %0, <i8 3, i8 7, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1>
  %b = call <8 x i8> @g(<8 x i8> %a)
  %c = add <8 x i8> %b, <i8 0, i8 7, i8 1, i8 1, i8 1, i8 1, i8 1, i8 100>
  ret <8 x i8> %c
}

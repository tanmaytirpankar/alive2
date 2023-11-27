define <16 x i8> @neg_v16i8(<16 x i8> %0) {
  %2 = sub <16 x i8> zeroinitializer, %0
  ret <16 x i8> %2
}

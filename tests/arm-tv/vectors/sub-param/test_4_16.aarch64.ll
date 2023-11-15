define <4 x i16> @vector_sub_4_16(<4 x i16> %a, <4 x i16> %b) {
    %c = sub <4 x i16> %a, %b
    ret <4 x i16> %c
}

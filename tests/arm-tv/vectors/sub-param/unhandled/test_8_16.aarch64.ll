define <8 x i16> @vector_sub_8_16(<8 x i16> %a, <8 x i16> %b) {
    %c = sub <8 x i16> %a, %b
    ret <8 x i16> %c
}

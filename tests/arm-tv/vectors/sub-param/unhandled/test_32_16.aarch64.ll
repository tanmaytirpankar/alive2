define <32 x i16> @vector_sub_32_16(<32 x i16> %a, <32 x i16> %b) {
    %c = sub <32 x i16> %a, %b
    ret <32 x i16> %c
}

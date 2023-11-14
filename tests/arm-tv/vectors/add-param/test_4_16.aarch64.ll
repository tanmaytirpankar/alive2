define <4 x i16> @vector_add_4_16(<4 x i16> %a, <4 x i16> %b) {
    %c = add <4 x i16> %a, %b
    ret <4 x i16> %c
}

define <8 x i16> @vector_add_8_16(<8 x i16> %a, <8 x i16> %b) {
    %c = add <8 x i16> %a, %b
    ret <8 x i16> %c
}

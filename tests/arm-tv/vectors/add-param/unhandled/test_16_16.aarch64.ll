define <16 x i16> @vector_add_16_16(<16 x i16> %a, <16 x i16> %b) {
    %c = add <16 x i16> %a, %b
    ret <16 x i16> %c
}

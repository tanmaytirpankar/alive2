define <2 x i16> @vector_add_2_16(<2 x i16> %a, <2 x i16> %b) {
    %c = add <2 x i16> %a, %b
    ret <2 x i16> %c
}

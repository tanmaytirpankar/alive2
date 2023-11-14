define <8 x i8> @vector_add_8_8(<8 x i8> %a, <8 x i8> %b) {
    %c = add <8 x i8> %a, %b
    ret <8 x i8> %c
}

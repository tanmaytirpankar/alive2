define <2 x i8> @vector_add_2_8(<2 x i8> %a, <2 x i8> %b) {
    %c = add <2 x i8> %a, %b
    ret <2 x i8> %c
}

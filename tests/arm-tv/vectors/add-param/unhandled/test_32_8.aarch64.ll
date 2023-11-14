define <32 x i8> @vector_add_32_8(<32 x i8> %a, <32 x i8> %b) {
    %c = add <32 x i8> %a, %b
    ret <32 x i8> %c
}

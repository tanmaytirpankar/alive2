define <64 x i8> @vector_add_64_8(<64 x i8> %a, <64 x i8> %b) {
    %c = add <64 x i8> %a, %b
    ret <64 x i8> %c
}

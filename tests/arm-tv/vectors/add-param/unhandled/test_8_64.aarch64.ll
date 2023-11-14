define <8 x i64> @vector_add_8_64(<8 x i64> %a, <8 x i64> %b) {
    %c = add <8 x i64> %a, %b
    ret <8 x i64> %c
}

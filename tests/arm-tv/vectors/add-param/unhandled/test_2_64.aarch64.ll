define <2 x i64> @vector_add_2_64(<2 x i64> %a, <2 x i64> %b) {
    %c = add <2 x i64> %a, %b
    ret <2 x i64> %c
}

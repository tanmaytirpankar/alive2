define <2 x i32> @vector_add_2_32(<2 x i32> %a, <2 x i32> %b) {
    %c = add <2 x i32> %a, %b
    ret <2 x i32> %c
}

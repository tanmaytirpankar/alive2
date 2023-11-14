define <16 x i32> @vector_add_16_32(<16 x i32> %a, <16 x i32> %b) {
    %c = add <16 x i32> %a, %b
    ret <16 x i32> %c
}

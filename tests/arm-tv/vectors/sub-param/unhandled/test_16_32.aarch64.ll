define <16 x i32> @vector_sub_16_32(<16 x i32> %a, <16 x i32> %b) {
    %c = sub <16 x i32> %a, %b
    ret <16 x i32> %c
}

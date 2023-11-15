define <8 x i32> @vector_sub_8_32(<8 x i32> %a, <8 x i32> %b) {
    %c = sub <8 x i32> %a, %b
    ret <8 x i32> %c
}

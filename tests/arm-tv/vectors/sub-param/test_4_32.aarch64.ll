define <4 x i32> @vector_sub_4_32(<4 x i32> %a, <4 x i32> %b) {
    %c = sub <4 x i32> %a, %b
    ret <4 x i32> %c
}

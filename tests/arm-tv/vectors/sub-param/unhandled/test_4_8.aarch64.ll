define <4 x i8> @vector_sub_4_8(<4 x i8> %a, <4 x i8> %b) {
    %c = sub <4 x i8> %a, %b
    ret <4 x i8> %c
}

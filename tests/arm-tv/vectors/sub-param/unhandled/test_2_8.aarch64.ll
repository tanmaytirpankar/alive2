define <2 x i8> @vector_sub_2_8(<2 x i8> %a, <2 x i8> %b) {
    %c = sub <2 x i8> %a, %b
    ret <2 x i8> %c
}

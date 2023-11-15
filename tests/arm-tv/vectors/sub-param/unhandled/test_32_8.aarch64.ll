define <32 x i8> @vector_sub_32_8(<32 x i8> %a, <32 x i8> %b) {
    %c = sub <32 x i8> %a, %b
    ret <32 x i8> %c
}

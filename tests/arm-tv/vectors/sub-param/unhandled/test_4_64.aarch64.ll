define <4 x i64> @vector_sub_4_64(<4 x i64> %a, <4 x i64> %b) {
    %c = sub <4 x i64> %a, %b
    ret <4 x i64> %c
}

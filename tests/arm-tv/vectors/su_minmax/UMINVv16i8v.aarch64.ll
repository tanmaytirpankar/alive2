define i1 @trunc_v16i8_cmp(<16 x i8> %0) {
  %2 = trunc <16 x i8> %0 to <16 x i1>
  %3 = bitcast <16 x i1> %2 to i16
  %4 = icmp ne i16 %3, -1
  ret i1 %4
}

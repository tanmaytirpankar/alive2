define <16 x i8> @unsigned_sat_variable_v16i8_using_min(<16 x i8> %0, <16 x i8> %1) {
  %3 = xor <16 x i8> %1, <i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1>
  %4 = icmp ult <16 x i8> %0, %3
  %5 = select <16 x i1> %4, <16 x i8> %0, <16 x i8> %3
  %6 = add <16 x i8> %5, %1
  ret <16 x i8> %6
}

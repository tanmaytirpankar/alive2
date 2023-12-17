define <2 x i32> @unsigned_sat_constant_using_min_splat(<2 x i32> %0) {
  %2 = icmp ult <2 x i32> %0, <i32 14, i32 14>
  %3 = select <2 x i1> %2, <2 x i32> %0, <2 x i32> <i32 14, i32 14>
  %4 = add <2 x i32> %3, <i32 -15, i32 -15>
  ret <2 x i32> %4
}

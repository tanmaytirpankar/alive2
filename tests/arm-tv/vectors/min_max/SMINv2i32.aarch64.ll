define <2 x i32> @max_of_nots_vec(<2 x i32> %0, <2 x i32> %1) {
  %3 = icmp sgt <2 x i32> %1, zeroinitializer
  %4 = xor <2 x i32> %1, <i32 -1, i32 -1>
  %5 = select <2 x i1> %3, <2 x i32> %4, <2 x i32> <i32 -1, i32 -1>
  %6 = xor <2 x i32> %0, <i32 -1, i32 -1>
  %7 = icmp slt <2 x i32> %5, %6
  %8 = select <2 x i1> %7, <2 x i32> %6, <2 x i32> %5
  ret <2 x i32> %8
}

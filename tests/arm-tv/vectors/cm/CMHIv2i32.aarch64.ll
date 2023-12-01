define <2 x i32> @uaddo_commute2(<2 x i32> %0, <2 x i32> %1, <2 x i32> %2) {
  %4 = xor <2 x i32> %1, <i32 -1, i32 -1>
  %5 = add <2 x i32> %1, %0
  %6 = icmp ugt <2 x i32> %0, %4
  %7 = select <2 x i1> %6, <2 x i32> %2, <2 x i32> %5
  ret <2 x i32> %7
}

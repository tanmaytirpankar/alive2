define <2 x i32> @f(<2 x i32> %0) {
  %2 = and <2 x i32> %0, <i32 6, i32 2>
  %3 = icmp eq <2 x i32> %2, zeroinitializer
  %4 = and <2 x i32> %0, <i32 1, i32 1>
  %5 = select <2 x i1> %3, <2 x i32> %4, <2 x i32> <i32 1, i32 1>
  ret <2 x i32> %5
}

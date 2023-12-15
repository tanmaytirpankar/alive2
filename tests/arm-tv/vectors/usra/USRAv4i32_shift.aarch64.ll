define <4 x i32> @usra4s(ptr %0, ptr %1) {
  %3 = load <4 x i32>, ptr %0, align 16
  %4 = lshr <4 x i32> %3, <i32 1, i32 1, i32 1, i32 1>
  %5 = load <4 x i32>, ptr %1, align 16
  %6 = add <4 x i32> %4, %5
  ret <4 x i32> %6
}

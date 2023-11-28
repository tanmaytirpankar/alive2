define <2 x i32> @v_shl_v2i32(<2 x i32> %0, <2 x i32> %1) {
  %3 = shl <2 x i32> %0, %1
  ret <2 x i32> %3
}

define <2 x i32> @f6(<2 x i32> %0) {
  %2 = add <2 x i32> %0, <i32 1, i32 1>
  ret <2 x i32> %2
}

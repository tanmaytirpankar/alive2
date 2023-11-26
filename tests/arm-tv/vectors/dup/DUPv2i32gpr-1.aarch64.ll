define <2 x i32> @fun39(<2 x i32> %0) {
  %2 = urem <2 x i32> %0, <i32 20, i32 20>
  ret <2 x i32> %2
}

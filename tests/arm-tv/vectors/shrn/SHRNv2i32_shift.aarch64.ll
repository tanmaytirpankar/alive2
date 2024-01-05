define <2 x i32> @f(ptr %0) {
  %2 = load <2 x i64>, ptr %0, align 16
  %3 = ashr <2 x i64> %2, <i64 32, i64 32>
  %4 = trunc <2 x i64> %3 to <2 x i32>
  ret <2 x i32> %4
}

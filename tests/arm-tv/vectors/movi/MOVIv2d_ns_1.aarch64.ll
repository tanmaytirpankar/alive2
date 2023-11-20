define <4 x i32> @srem_poison_lhs_vec(<4 x i32> %0) {
  %2 = srem <4 x i32> poison, %0
  ret <4 x i32> %2
}

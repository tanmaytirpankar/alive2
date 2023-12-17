define <2 x i32> @canonicalize_logic_first_xor_vector0(<2 x i32> %0) {
  %2 = add <2 x i32> <i32 -8388608, i32 -8388608>, %0
  %3 = xor <2 x i32> <i32 32783, i32 32783>, %2
  ret <2 x i32> %3
}

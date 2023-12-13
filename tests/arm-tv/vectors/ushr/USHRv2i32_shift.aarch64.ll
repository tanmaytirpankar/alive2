define void @test_udiv_pow2_v2i32(ptr %0, ptr %1) {
  %3 = load <2 x i32>, ptr %0, align 8
  %4 = udiv <2 x i32> %3, <i32 8, i32 8>
  store <2 x i32> %4, ptr %1, align 8
  ret void
}

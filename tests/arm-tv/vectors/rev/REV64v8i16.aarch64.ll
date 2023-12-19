define void @test_rev_fail(ptr %0) {
  %2 = load <16 x i16>, ptr %0, align 32
  %3 = shufflevector <16 x i16> %2, <16 x i16> undef, <16 x i32> <i32 7, i32 6, i32 5, i32 4, i32 3, i32 2, i32 1, i32 0, i32 15, i32 14, i32 13, i32 12, i32 11, i32 10, i32 9, i32 8>
  store <16 x i16> %3, ptr %0, align 32
  ret void
}

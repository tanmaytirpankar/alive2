define void @sext_v16i8_v16i32(<16 x i8> %0, ptr %1) {
  %3 = sext <16 x i8> %0 to <16 x i32>
  store <16 x i32> %3, ptr %1, align 64
  ret void
}

define void @insertelt_v8i64_store(ptr %0, i32 %1) {
  %3 = load <8 x i64>, ptr %0, align 64
  %4 = insertelement <8 x i64> %3, i64 -1, i32 %1
  store <8 x i64> %4, ptr %0, align 64
  ret void
}

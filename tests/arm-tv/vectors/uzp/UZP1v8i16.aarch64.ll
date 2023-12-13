define void @func_cvt51(ptr %0, ptr %1) {
  %3 = load <8 x i32>, ptr %0, align 32
  %4 = trunc <8 x i32> %3 to <8 x i8>
  store <8 x i8> %4, ptr %1, align 8
  ret void
}

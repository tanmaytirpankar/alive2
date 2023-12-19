define i32 @shuf_load_4bytes(ptr %0) {
  %2 = load <4 x i8>, ptr %0, align 4
  %3 = shufflevector <4 x i8> %2, <4 x i8> undef, <4 x i32> <i32 3, i32 2, i32 poison, i32 0>
  %4 = bitcast <4 x i8> %3 to i32
  ret i32 %4
}
